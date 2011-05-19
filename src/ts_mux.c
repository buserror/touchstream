/*
	ts_mux.c

	Copyright 2011 Michel Pollet <buserror@gmail.com>

 	This file is part of touchstream.

	touchstream is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	touchstream is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with touchstream.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * the packet format is trivial on the link, it's
 * a zero terminated string; the 'zero' is also a packet delimiter,
 * and therefore can't exist in the payload.
 * There is no check for now if for example the clipboard contains zero,
 * theoricaly, it's UTF8, so there shouldn't.
 *
 * A example packet is as follow:
 * Svx1w1920h1200nyelp
 * Means
 * 'S' is packet type, in this case a Server screen notification
 * 'vx1' is version hex '1' -- any suite of hex char is read
 * 'w1920' sets the width to 1920
 * 'h1200' sets the height to 1200
 * nyelp sets the name to 'yelp' -- ends at the end of the packet, or ':'
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <netdb.h>
#include <ctype.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/select.h>

#include "ts_mux.h"
#include "ts_display_proxy.h"
#include "ts_verbose.h"

#define TS_MUX_VERSION 0x0001

DEFINE_FIFO(ts_display_proxy_event_t, proxy_fifo, 32);

#define _MAX(a, b) ((a) > (b) ? (a) : (b))

/*
 * Mux/demux thread
 *
 * This thread waits for events on it's signal socket or on any of the
 * 'remote' sockets, and then calls their callbacks to dispatch the event.
 *
 * Note that the mux thread is used with network socket as expected, but is
 * also used for xorg "Connection" and ts_signal socketpair(), it makes sure
 * that all the code runs on a single thread, and prevents the need for
 * mutexes and other garbage.
 */
static void *
_sn_mux_thread(
		void * ignore)
{
	ts_mux_p mux = (ts_mux_p)ignore;

	fd_set readSet, writeSet;
	while (1) {
		int max = -1;
		FD_ZERO(&readSet);
		FD_ZERO(&writeSet);

		FD_SET(mux->signal.fd[TS_SIGNAL_END1], &readSet);
		max = _MAX(max, mux->signal.fd[TS_SIGNAL_END1]);

		/*
		 * Mark all the remotes ready to read, also check to see if
		 * they have outgoing data, if so, also check for an event
		 * to make sure socket is ready to send before sending out
		 */
		for (int i = 0; i < 32; i++) {
			if ((mux->dp_usage & (1 << i))) {
				ts_remote_p fun = mux->dp[i];
				if (fun->socket <= 0 && fun->start(fun))
					continue;

				if (!fun->can_read || fun->can_read(fun)) {
					FD_SET(fun->socket, &readSet);
					max = _MAX(max, fun->socket);
				}

				if (fun->can_write && fun->can_write(fun)) {
					FD_SET(fun->socket, &writeSet);
					max = _MAX(max, fun->socket);
				}
			}
		}

		/*
		 * Wait for an event on any of the sockets
		 */
		struct timeval timo = { .tv_sec = 1, .tv_usec = 0 };
		/*int ret = */ select(max + 1, &readSet, &writeSet, NULL, &timo);

		// have we been signaled ?
		if (FD_ISSET(mux->signal.fd[TS_SIGNAL_END1], &readSet)) {
			// flush socket
			ts_signal_flush(&mux->signal, TS_SIGNAL_END1);
		//	printf("%s: signaled\n", __FUNCTION__);
		}
		for (int i = 0; i < 32; i++)
			if ((mux->dp_usage & (1 << i))) {
				ts_remote_p fun = mux->dp[i];
				if (fun->socket <= 0) /* not yet connected anyway */
					continue;
				int rd = FD_ISSET(fun->socket, &readSet);
				int wr = FD_ISSET(fun->socket, &writeSet);
				if (rd || wr)
					V3("slot %d %p socket %d rd %d wr %d\n",
						i, fun, fun->socket, rd, wr);
				if (rd && fun->data_read)
					fun->data_read(fun);
				if (wr && fun->data_write)
					fun->data_write(fun);
			}
	}
	return NULL;
}

int
ts_mux_start(
		ts_mux_p mux,
		ts_master_p master )
{
	if (mux->thread)
		return 0;
	mux->master = master;
	/*
	 * Create the socket pair for signaling the mux thread
	 */
	ts_signal_init(&mux->signal);
	/*
	 * Create the thread
	 */
	if (pthread_create(&mux->thread, NULL,
			_sn_mux_thread, mux)) {
		perror("_sn_server_mux_create pthread_create");
		mux->thread = 0;
		return -1;
	}
	return 0;
}

//! Signal the thread side from the client side
void
ts_mux_signal(
		ts_mux_p mux,
		uint8_t what )
{
	ts_signal(&mux->signal, TS_SIGNAL_END0, 0);
}

/*
 * These are not thread safe, fortunately, they should only happend
 * before the thread is started OR in the mux's thread context.
 */
int
ts_mux_register(
		ts_remote_p r)
{
	for (int i = 0; i < 32; i++)
		if (!(r->mux->dp_usage & (1 << i))) {
			r->mux->dp[i] = r;
			r->mux->dp_usage |= 1 << i;
			ts_mux_signal(r->mux, 0);
			return 0;
		}
	return -1;
}

int
ts_mux_unregister(
		ts_remote_p r)
{
	for (int i = 0; i < 32; i++)
		if ((r->mux->dp_usage & (1 << i)) && r->mux->dp[i] == r) {
			r->mux->dp_usage &= ~(1 << i);
			r->mux->dp[i] = NULL;
//			ts_mux_signal(r->mux, 0);
			return 0;
		}
	return -1;
}

/*
 * Create an outgoing socket, and attempts to connect to it asynchronously
 */
static int
connect_start(
		struct ts_remote_t * r)
{
	if (r->timeout && time(NULL) - r->timeout < 5)
		return -1;

	int skt = socket(AF_INET, SOCK_STREAM, 0);
	if (skt < 0)
		return -1;
	size_t addrLen = sizeof(r->addr);

	r->state = skt_state_Connect;
    int i = 1;
    // we can ignore error here, on UNIX sockets
    setsockopt (skt, IPPROTO_TCP, TCP_NODELAY, &i, sizeof (i));

    r->timeout = 0;
	r->socket = skt;
	{	// make it nonblocking
		int flags = fcntl(r->socket, F_GETFL, 0);
		fcntl(r->socket, F_SETFL, flags | O_NONBLOCK);
	}

    if (connect(skt, (struct sockaddr*)&r->addr, addrLen) < 0) {
    	if (errno != EINPROGRESS) {
			perror("connect_start");
			close(skt);
			return -1;
    	}
    	V1("Connection in progress (%s)\n", __func__);
    }

	return 0;
}

/*
 * if a remote socket fails, delete it, and set ourself up
 * ready to re-attempt connection in a few seconds.
 */
static int
connect_restart(
		struct ts_remote_t * r)
{
	V1("Outgoing connection retrying (%s)\n", __func__);
	r->timeout = time(NULL);
	/*
	 * Note, we do NOT delete the r->display here, as outgoing socket's hold
	 * the 'main' display there, so we just close the socket and try to
	 * reconnect to the server instead.
	 */
	close(r->socket);
	r->socket = -1;
	return -1;
}

/*
 * This is called when the socket has been truly established.
 * Theoricaly, we could use the existing system to buffer & send
 * our message, but since we /just started/ we can assume there's
 * a few bytes free in the socket's buffer for a write() to work!
 */
static int
connect_established(
		struct ts_remote_t * r)
{
	V1("Outgoing connection established (%s)\n", __func__);
	ts_display_p d = r->display;
	char msg[32];
	sprintf(msg, "Cvx%xw%dh%dn%s:p%s:", TS_MUX_VERSION,
			d->bounds.w, d->bounds.h, d->name,
			d->param ? d->param : "");
	write(r->socket, msg, strlen(msg)+1);
	return 0;
}

/*
 * If the outgoing socket is not established, do not
 * try to register for read events, it won't work
 */
static int
connect_can_read(
		struct ts_remote_t * r)
{
	if (r->state == skt_state_Connect) {
		return 0;
	}
	return 1;
}

/*
 * Check to see whether we want to write anything. It could be
 * + That we are still establishing the connection, therefore we
 *   need to register for this event as per select() requirement
 * + We have an outgoing buffer that still has some data not sent
 *   from the last time we tried to send it
 * + There are new events in the FIFO to convert into packets
 */
static int
connect_can_write(
		struct ts_remote_t * r)
{
	if (r->state == skt_state_Connect) {
		return 1;
	}
	if (r->out_len)
		return 1;
	// check fifo...
	if (!r->proxy)
		return 0;

	return !proxy_fifo_isempty(&r->proxy->fifo);
}

/*
 * This is called when an incoming socket has been established (from the listen one)
 * It means we are a 'server' and therefore we send a 'server' packet quickly
 * with the geometry of the screen
 */
static int
data_start(
		struct ts_remote_t * r)
{
	r->socket = r->accept_socket;
	V2("%s Incoming connection socket %d\n", __func__, r->socket);
	ts_display_p d = ts_master_get_main(r->mux->master);
	char msg[32];
	sprintf(msg, "Svx%xw%dh%dn%s", TS_MUX_VERSION, d->bounds.w, d->bounds.h, d->name);
	write(r->socket, msg, strlen(msg)+1);
	return 0;
}

/*
 * Incoming socket has closed down, we need to tear down any proxy display
 * we had, clean the buffers, unregister ourselves from the mux and die.
 */
static int
data_restart(
		struct ts_remote_t * r)
{
	V1("Incoming connection to %s terminated (%s)\n",
			r->display ? r->display->name : "(unknown)",__func__);
	if (r->display) {
		ts_master_display_remove(r->display->master, r->display);
		r->display = NULL;
	}
	ts_mux_unregister(r);
	if (r->in)
		free(r->in);
	r->in = NULL;
	r->in_size = r->in_len = 0;
	if (r->out)
		free(r->out);
	r->out = NULL;
	r->out_size = r->out_len = 0;
	if (r->dispose)
		r->dispose(r);
	else {
		close(r->socket);
		free(r);
	}
	return -1;
}


/*
 * Check to see wether we want to write anything. It could be
 * + We have an outgoing buffer that still has some data not sent
 *   from the last time we tried to send it
 * + There are new events in the FIFO to convert into packets
 */
static int
data_can_write(
		struct ts_remote_t * r)
{
	if (r->out_len)
		return 1;
	if (!r->proxy)
		return 0;

	return !proxy_fifo_isempty(&r->proxy->fifo);
}

/* INTERNAL PACKET DECODING UTILITY
 * extract one numerical parameter from the string, advance the pointer.
 * format is either 'xXXXXXX' hex or 'DDDD'
 */
static int
data_get_integer(
		uint8_t ** o)
{
	int hex = 0;
	int minus = 1;
	uint8_t *p = *o;

	if (*p == 'x') {
		hex = 1; p++;
	} else if (*p == '-') {
		minus = -1; p++;
	}
	int res = 0;
	if (hex) {
		while (isxdigit(*p)) {
			char c = tolower(*p);
			if (isdigit(c))
				res = (res << 4) | (c - '0');
			else
				res = (res << 4) | (c - 'a' + 10);
			p++;
		}
	} else {
		while (isdigit(*p)) {
			res = (res * 10) + (*p - '0');
			p++;
		}
	}
	*o = p;
	return res * minus;
}

/* INTERNAL PACKET DECODING UTILITY
 * Reads a string, terminated with zero or a specified
 */
static char *
data_get_string(
		uint8_t **o,
		char term )
{
	uint8_t * p = *o;
	while (*p && *p != term)
		p++;
	if (*p) {
		*p = 0; p++;
	}
	uint8_t * res = *o;
	*o = p;
	return (char*)res;
}


/* INTERNAL PACKET UTILITY
 * Attemps to write as much as possible from our buffer to the socket.
 * Then move the remaining at the start of the buffer until it's empty
 * return the bytes remaining
 */
static int
data_event_write_flush(
		struct ts_remote_t * r)
{
	if (!r->out_len)
		return 0;

	ssize_t ss = write(r->socket, r->out, r->out_len);
	if (ss < 0)
		return -1;
	if (ss == r->out_len)
		r->out_len = 0;
	else {
		memmove(r->out, r->out + ss, r->out_len - ss);
		r->out_len -= ss;
	}

	return r->out_len;
}

/* INTERNAL PACKET UTILITY
 * Tries to make sure there's at least 'size' bytes in the buffer, and return
 * a pointer to the place we allocated.
 */
static uint8_t *
data_event_write_alloc(
		struct ts_remote_t * r,
		int size )
{
	if (r->out_len + size >= r->out_size) {
		int news = (r->out_len + size + 128) & ~127;
		V3("%s buffer reallocated to %d\n", __func__, (int)news);
		r->out = realloc(r->out, news);
		r->out_size = news;
	}
	*(r->out + r->out_len) = 0;
	return r->out + r->out_len;
}

static uint8_t *
data_event_write_commit(
		struct ts_remote_t * r,
		uint8_t * buf )
{
	if (!buf || !*buf)
		return NULL;
	int l = strlen((char*)buf);
	r->out_len += l + 1;
	return NULL;
}

/*
 * This look for text in a clipboard, and generate packets to
 * + clear remote clipboard named 'name'
 * + set the text flavors
 * + set the clipboard once it is fully sent
 */
static void
data_event_write_clipboard(
		struct ts_remote_t * r,
		ts_clipboard_p clipboard,
		char * name)
{
	uint8_t * buf = data_event_write_alloc(r, 16);
	sprintf((char*)buf, "cn%s", name);
	buf = data_event_write_commit(r, buf);
	for (int i = 0; i < clipboard->flavorCount; i++)
		if (!strncmp(clipboard->flavor[i].name, "text", 4)) {
			buf = data_event_write_alloc(r, clipboard->flavor[i].size + 32);
			sprintf((char*)buf, "fn%s:F%s:D%s", name,
					clipboard->flavor[i].name,
					clipboard->flavor[i].data);
			buf = data_event_write_commit(r, buf);
		}
	buf = data_event_write_alloc(r, 16);
	sprintf((char*)buf, "sn%s", name);
	buf = data_event_write_commit(r, buf);
}

/*
 * Pools the display event fifo, takes the events from there, and packetize
 * them into an output buffer, they are then sent as fast as the socket will
 * allow.
 */
static int
data_event_write(
		struct ts_remote_t * r)
{
	/*
	 * During an outgoing connection to a server, we monitor the socket
	 * state for a "write event", this is the select() convention
	 * Once a write event has been received, check for any error, and
	 * if all is well transform into a normal "data" one...
	 */
	if (r->state == skt_state_Connect) {
	    int e = 1;
	    socklen_t s = sizeof(e);
	    // we can ignore error here, on UNIX sockets
	    getsockopt (r->socket, SOL_SOCKET, SO_ERROR, &e, &s);
	//	printf("data_event_write connection finished error %d\n", e );
		if (e) {
			errno = e;
			perror("connect failed: ");
			connect_restart(r);
			return 0;
		}
		r->state = skt_state_Data;
		V2("%s connection established\n", __func__);
		connect_established(r);
	}
	/*
	 * if there is a buffer with stuff in already, send it off
	 */
	if (data_event_write_flush(r))
		return 0;
	if (!r->proxy)
		return 0;

	/*
	 * Empty the fifo, packetize anything we have
	 */
	while (!proxy_fifo_isempty(&r->proxy->fifo)) {
		ts_display_proxy_event_t e = proxy_fifo_read(&r->proxy->fifo);
		uint8_t * buf = NULL;
		switch (e.event) {
			case ts_proxy_init:
			//	d->slave->init(display);
				break;
			case ts_proxy_dispose:
			//	d->slave->dispose(display);
				break;
			case ts_proxy_enter:
			//	d->slave->enter(display, e.u.display, e.flags);
				buf = data_event_write_alloc(r, 32);
				sprintf((char*)buf, "ex%dy%d",
						r->display->mousex, r->display->mousey);
				V3("%s: %s\n", __func__, (char*)buf);
				break;
			case ts_proxy_leave:
				buf = data_event_write_alloc(r, 8);
				sprintf((char*)buf, "l");
				break;
			case ts_proxy_mouse:
				buf = data_event_write_alloc(r, 32);
				sprintf((char*)buf, "mx%dy%d", e.u.mouse.x,  e.u.mouse.y);
				break;
			case ts_proxy_button:
				buf = data_event_write_alloc(r, 16);
				sprintf((char*)buf, "bb%dd%d", e.u.button, e.down);
				break;
			case ts_proxy_key:
				buf = data_event_write_alloc(r, 16);
				sprintf((char*)buf, "kd%dkx%04x", e.down, e.u.key);
				break;
			case ts_proxy_wheel:
				buf = data_event_write_alloc(r, 16);
				sprintf((char*)buf, "wb%dx%dy%d", e.u.wheel.wheel, e.u.wheel.x, e.u.wheel.y);
				break;
			case ts_proxy_getclipboard:
				buf = data_event_write_alloc(r, 8);
				sprintf((char*)buf, "gn%s", ts_master_get_main(r->mux->master)->name);
				break;
			case ts_proxy_setclipboard: {
				if (!e.u.clipboard || !e.u.clipboard->flavorCount)
					break;
				data_event_write_clipboard(r,
						e.u.clipboard, ts_master_get_main(r->mux->master)->name);
			}	break;
		}
		data_event_write_commit(r, buf);
	}
	/*
	 * we must have generated packets there, so try to send them now too, instead
	 * of waiting for another select() event.
	 */
	 data_event_write_flush(r);
	return 0;
}

/*
 * This is called when our remote master "screen" wants the clipboard;
 * so we do just that, we serialize it, and we send it off
 */
static void ts_mux_remote_setclipboard(struct ts_display_t *d, ts_clipboard_p clipboard)
{
	ts_remote_p r = d->driver->refCon;
	V3("%s\n", __func__);

	data_event_write_clipboard(r, clipboard, d->name);
	ts_mux_signal(r->mux, 0);
}

static ts_display_driver_t ts_mux_driver_remote = {
		.setclipboard = ts_mux_remote_setclipboard,
};

/*
 * Receive a fully formed packet -- refer to the top of the file for a
 * bit more about the packet format.
 * One thing of note is that the parameter 'names' is predefined, and the
 * parser try to decode as many as it can before looking at the data type.
 * Since many packets share parameter names/type, it makes the code a
 * lot simpler.
 */
static void
data_process_packet(
		struct ts_remote_t * r,
		uint8_t * pkt,
		size_t len )
{
	uint8_t * p = pkt;
	char kind = *p++;

	int v = 0 , w = 0, h = 0;
	int x = 0, y = 0;
	int b = 0, d = 0;
	uint16_t k = 0;
	char * param = NULL;
	char * name = NULL;
	char * flavor = NULL;
	char * data = NULL;

//	printf("packet '%s'\n", pkt);
	/*
	 * Try to decode all the parameters we find
	 */
	int ok = 1;
	while (*p && ok) {
		switch (*p) {
			case 'v': p++; v = data_get_integer(&p); break; // version
			case 'w': p++; w = data_get_integer(&p); break; // width
			case 'h': p++; h = data_get_integer(&p); break; // height
			case 'p': p++; param = data_get_string(&p, ':'); break; // param
			case 'n': p++; name = data_get_string(&p, ':'); break; // name
			case 'F': p++; flavor = data_get_string(&p, ':'); break; // flavor string
			case 'D': p++; data = data_get_string(&p, 0); break; // data, always zero termed
			case 'x': p++; x = data_get_integer(&p); break;
			case 'y': p++; y = data_get_integer(&p); break;
			case 'b': p++; b = data_get_integer(&p); break; // button
			case 'd': p++; d = data_get_integer(&p); break; // down/up
			case 'k': p++; k = data_get_integer(&p); break; // key (unsigned)
			default: ok = 0;
		}
	}

	switch (kind) {
		case 'C':	// client screen
		case 'S': {	// server screen
			V3("packet %s %s version %04x screen %4dx%4d param '%s'\n",
					kind == 's' ? "server" : "client", name, v, w, h, param ? (char*)param : "(none)");
			if (!(kind && w && h)) {
				V1("%s invalid '%c' packet anyway\n", __func__, kind);
				break;
			}

			ts_display_driver_p driver = NULL;
			if (kind == 'C') {
				// we're the server, let's setup a proxy screen to start making packets
				V1("Setting up new client screen '%s' (%s) \n", name, __func__);

				driver = ts_display_proxy_driver(r->mux, NULL);
				r->proxy = (ts_display_proxy_driver_p)driver;
				r->proxy->signal.fd[TS_SIGNAL_END0] = r->mux->signal.fd[TS_SIGNAL_END0];
			} else {
				V1("Setting server screen '%s' (%s) \n", name, __func__);
				driver = ts_display_clone_driver(&ts_mux_driver_remote);
				driver->refCon = r;
				param = r->display->param;
			}
			ts_display_p new_display = malloc(sizeof(ts_display_t));
			ts_display_init(new_display, r->mux->master, driver, name, param);
			new_display->bounds.w = w;
			new_display->bounds.h = h;
			ts_master_display_add(r->mux->master, new_display);

			if (kind == 'C') {
				r->display = new_display;
				ts_display_place(
						ts_master_get_main(r->mux->master),
						new_display, param);
			} else {
				// we're a client, we're just happy about life and getting events!
				// we still attach a screen for the "server", so the mouse warp is easier
				ts_display_place(
						new_display,
						ts_master_get_main(r->mux->master), param);
			}
		}	break;
		case 'm': {	// mouse move
			if (r->proxy)
				break;
			ts_display_movemouse(ts_master_get_main(r->mux->master), x, y);
		}	break;
		case 'b': {	// mouse move
			if (r->proxy)
				break;
			ts_display_button(ts_master_get_main(r->mux->master), b, d);
		}	break;
		case 'w': {	// mouse wheel
			if (r->proxy)
				break;
			ts_display_wheel(ts_master_get_main(r->mux->master), b, y, x);
		}	break;
		case 'k': {	// key
			if (r->proxy)
				break;
			ts_display_key(ts_master_get_main(r->mux->master), k, d);
		}	break;
		case 'e': {	// enter
			if (r->proxy)
				break;
			ts_display_p dd = ts_master_get_main(r->mux->master);
			if (dd) {
				// update the coordinate on the fake "old" screen, so mouse warping works
				dd->master->mousex = dd->bounds.x + x;
				dd->master->mousey = dd->bounds.y + y;
				ts_display_enter(dd);
			}
		}	break;
		case 'l': {	// leave
			if (r->proxy)
				break;
			ts_display_leave(ts_master_get_main(r->mux->master));
		}	break;
		case 'g': {	// getclipboard
			V3("%s get clipboard\n", __func__);
			if (r->proxy)
				break;
			ts_display_p target = ts_master_display_get(r->mux->master, name);
			ts_display_getclipboard(
					ts_master_get_main(r->mux->master),
					target);
		}	break;
		case 'c': {	// clear clipboard
			V3("%s clear clipboard\n", __func__);
			ts_display_p target = name ?
					ts_master_display_get(r->mux->master, name) :
					ts_master_get_main(r->mux->master);
			if (target)
				ts_clipboard_clear(&target->clipboard);
		}	break;
		case 'f': {	// clipboard flavor
			V3("%s clipboard flavor\n", __func__);
			ts_display_p target = name ?
					ts_master_display_get(r->mux->master, name) :
					ts_master_get_main(r->mux->master);
			if (target)
				ts_clipboard_add(&target->clipboard, flavor, strlen(data), data);
		}	break;
		case 's': {	// set clipboard
			V3("%s set clipboard\n", __func__);
			ts_display_p target = name ?
					ts_master_display_get(r->mux->master, name) :
					ts_master_get_main(r->mux->master);
			if (target)
				ts_display_setclipboard(
					ts_master_get_main(r->mux->master),
					&target->clipboard);
		}	break;
		default:
			V1("%s unknown packet kind '%c'\n", __func__, kind);
			break;
	}
}

/*
 * read some data, we first save it in a growing buffer, until we find a zero.
 * Then we process that packet and continue on for each line.
 */
static int
data_event_read(
		struct ts_remote_t * r)
{
	uint8_t in[256];
	ssize_t ss = 0;
	do {
		ss = read(r->socket, in, sizeof(in));

		/*
		 * Error, or disconnect, we drop this link
		 */
		if (ss <= 0) {
			if (r->restart)
				r->restart(r);
			//sleep(1);
			return -1;
		}
		/*
		 * Try to add this in to the 'current' buffer, grow it if necessary
		 */
		//printf("%s in len %d, ss %d\n",__func__, r->in_len, (int)ss);
		if (r->in_len + ss > r->in_size) {
			int news = (r->in_len + ss + 128) & ~127;
			r->in = realloc(r->in, news);
			V3("%s reallocated in from %d to %d\n", __func__, r->in_size, news);
			r->in_size = news;
		}
		memcpy(r->in + r->in_len, in, ss);
		r->in_len += ss;

		/*
		 * try to find 'packets' from the current input buffer
		 */
		int packet_len = 0;
		do {
			packet_len = 0;

			for (int bi = 0; bi < r->in_len; bi++)
				if (r->in[bi] == 0) {
					packet_len = bi;
					break;
				}

			/*
			 * if a packet is found, move the remaining and continue
			 */
			if (packet_len) {
				// got a packet
			//	printf("%s found a packet %d size (%d in in)\n", __func__, packet_len, r->in_len);

				if (packet_len) {
					r->in[packet_len] = 0;
					if (packet_len > 0)
						data_process_packet(r, r->in, packet_len);
					/*
					 * eat up any remaining line terminations etc
					 */
					packet_len++;
					while (packet_len < r->in_len && r->in[packet_len] < ' ')
						packet_len++;
				}
				/* move remaining bytes */
				if (r->in_len > packet_len)
					memmove(r->in,
							r->in + packet_len,
							r->in_len - packet_len);
				r->in_len -= packet_len;
			//	printf("%s processed packet, %d remains\n", __func__, r->in_len);
			}
		} while (packet_len);

	} while (ss == sizeof(in));
	return 0;
}

/*
 * Starts a listen socket
 */
static int
listen_start(
		struct ts_remote_t * r)
{
	int skt = socket(AF_INET, SOCK_STREAM, 0);
	if (skt < 0)
		return -1;
	int optval = 1;
	setsockopt(skt, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

	size_t addrLen = sizeof(r->addr);

	if (bind(skt, (struct sockaddr*)&r->addr, addrLen) < 0) {
		perror("listen_start bind");
		close(skt);
		return -1;
	}
    if (listen(skt, 10) < 0) {
    	perror("listen_start listen");
    	close(skt);
    	return -1;
    }
    r->state = skt_state_Listen;
	r->socket = skt;

	{	// make it nonblocking
		int flags = fcntl(skt, F_GETFL, 0);
		fcntl(skt, F_SETFL, flags | O_NONBLOCK);
	}
	return 0;
}

/*
 * failure on a listen socket is bad news anyway
 */
static int
listen_restart(
		struct ts_remote_t * r)
{
	// we could close the socket and force a start() here...
	printf("%s can't recover easily from that. dying\n", __FUNCTION__);
	exit(1);
}

/*
 * A 'read' event on a listen socket means we for a connection, so accept it,
 * generate a new 'data' remote structure, and add it to the mux
 */
static int
listen_event_read(
		struct ts_remote_t * r)
{
	V2("%s accepting on %d\n", __func__, r->socket);
	struct sockaddr_in  addr;
	socklen_t addrLen = sizeof(addr);
	int fd = accept(r->socket, (struct sockaddr*)&addr, &addrLen);
	if (fd <= 0) {
		perror("_vdmsg_funnel_event_listen accept");
		exit(1);
		return -1;
	}
    int i = 1;
    setsockopt (fd, IPPROTO_TCP, TCP_NODELAY, &i, sizeof (i));

	ts_remote_p res = malloc(sizeof(ts_remote_t));
	memset(res, 0, sizeof(*res));
	res->addr = addr;
	res->accept_socket = fd;
	res->start = data_start;
	res->restart = data_restart;
	res->can_write = data_can_write;
	res->data_read = data_event_read;
	res->data_write = data_event_write;
	res->mux = r->mux;

	ts_mux_register(res);

	return 0;
}

/*
 * Create a new remote on the current mux. It can be either a listen one
 * (if address is NULL) or an outgoing one if it isn't NULL.
 */
int
ts_mux_port_new(
		ts_mux_p mux,
		ts_master_p master,
		char * address,
		ts_display_p display)
{
	ts_remote_p res = malloc(sizeof(ts_remote_t));
	memset(res, 0, sizeof(*res));

	res->mux = mux;
	res->addr.sin_family = AF_INET;
	res->addr.sin_addr.s_addr = INADDR_ANY;
	res->addr.sin_port = htons(1869);
	res->display = display;

	if (address) {
		char *port = strchr(address, ':');
		if (port) {
			*port = 0; port++;
			res->addr.sin_port = htons(atoi(port));
		}
	    struct hostent *hp = gethostbyname(address);
	    if (!hp) {
	    	fprintf(stderr, "%s unknown host '%s'\n", __func__, address);
	    	exit(1);
	    }
		res->addr.sin_addr.s_addr = *(in_addr_t *) (hp->h_addr_list[0]);

		res->start = connect_start;
		res->restart = connect_restart;
		res->can_read = connect_can_read;
		res->can_write = connect_can_write;
		res->data_read = data_event_read;
		res->data_write = data_event_write;
	} else {
		res->start = listen_start;
		res->restart = listen_restart;
		res->data_read = listen_event_read;
	}

	// start the mux, if it wasn't there already
	ts_mux_start(mux, master);
	// register the remote with the mux
	ts_mux_register(res);

	return -1;
}

