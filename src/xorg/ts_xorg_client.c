/*
	ts_xorg_client.c

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

#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <ctype.h>
#include <netinet/tcp.h>

#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/dpms.h>
#include <X11/Xatom.h> /* for clipboard */

#include "ts_defines.h"
#include "ts_mux.h"
#include "ts_xorg.h"
#include "ts_display_proxy.h"
#include "ts_verbose.h"

typedef struct ts_xorg_client_t {
	ts_display_t display;
	ts_mux_p mux;
	char * displayname;
	Display * dp;
	Window	root;
	Window window;

	ts_remote_t remote;
	ts_xorg_krev_t map;
} ts_xorg_client_t, *ts_xorg_client_p;

#define ERR_BUF_SIZE 1024
int
x11_error_handler(
		Display* dpy,
		XErrorEvent* ee)
{
	char error_msg[ERR_BUF_SIZE] = "";
	char message[ERR_BUF_SIZE] = "";
	char default_string[ERR_BUF_SIZE] = "";
	XGetErrorText(dpy, ee->error_code, error_msg, ERR_BUF_SIZE);
//	XGetErrorDatabaseText(dpy, "XDAMAGE", message, "", error_msg, ERR_BUF_SIZE);
	fprintf(stderr, "** error received from X server: %s\n%s\n%s\n", message, default_string,
			error_msg);
	return 0;
}

static Cursor
createBlankCursor(
		ts_xorg_client_p d)
{
	// this seems just a bit more complicated than really necessary

	// get the closet cursor size to 1x1
	unsigned int w, h;
	XQueryBestCursor(d->dp, d->root, 1, 1, &w, &h);

	// make bitmap data for cursor of closet size.  since the cursor
	// is blank we can use the same bitmap for shape and mask:  all
	// zeros.
	const int size = ((w + 7) >> 3) * h;
	char* data = malloc(size);
	memset(data, 0, size);

	// make bitmap
	Pixmap bitmap = XCreateBitmapFromData(d->dp, d->root, data, w, h);

	// need an arbitrary color for the cursor
	XColor color;
	color.pixel = 0;
	color.red   = color.green = color.blue = 0;
	color.flags = DoRed | DoGreen | DoBlue;

	// make cursor from bitmap
	Cursor cursor = XCreatePixmapCursor(d->dp, bitmap, bitmap,
								&color, &color, 0, 0);

	// don't need bitmap or the data anymore
	free(data);
	XFreePixmap(d->dp, bitmap);

	return cursor;
}

static Atom XA_CLIPBOARD;

static int
xorg_client_eventloop(
		struct ts_remote_t * r)
{
	ts_xorg_client_p d = (ts_xorg_client_p)r->display;

//	printf("%s\n", __func__);
	XEvent e;

	while (XPending(d->dp)) {
		XNextEvent(d->dp, &e);
		if (e.type == SelectionRequest) {
			V2("%s SelectionRequest out window is %x\n",__func__, (int)d->window);
			V3("Requester=%x selection=%d target=%d property=%d\n",
					(int)e.xselectionrequest.requestor,
					(int)e.xselectionrequest.selection,
					(int)e.xselectionrequest.target,
					(int)e.xselectionrequest.property);

			V3("target=%s\n",
					XGetAtomName(d->dp,e.xselectionrequest.target));
			V3("property=%s\n",
			        XGetAtomName(d->dp,
			                e.xselectionrequest.property));

			int result;
			if (e.xselectionrequest.requestor != d->window) {

				Atom type;
			//	XSelectInput(dpy, w, StructureNotifyMask+ExposureMask);
				int format;
				unsigned long len, bytes_left;
				unsigned char *data = NULL;

				result = XGetWindowProperty(d->dp, d->window,
						XA_PRIMARY, 0, 10000000L, 0,
						XA_STRING, &type, &format, &len, &bytes_left, &data);
				V3("twin primary type=%d format=%d len=%d data=%p\n",
						(int) type, format, (int) len, data);
				if (result == Success) {
					//Try to change the property
					//Put the XA_PRIMARY string into the requested property of
					//requesting window

					result = XChangeProperty(d->dp, e.xselectionrequest.requestor,
							e.xselectionrequest.property, e.xselectionrequest.target,
							8, PropModeReplace, (unsigned char *) data, len);
					if (result == BadAlloc || result == BadAtom || result == BadMatch
							|| result == BadValue || result == BadWindow) {
						fprintf(stderr, "XChangeProperty failed %d\n", result);
					}
				}
			}
				//free(test);

			XSelectionEvent xev;
			//make SelectionNotify event
			xev.type = SelectionNotify;
			xev.send_event = True;
			xev.display = d->dp;
			xev.requestor = e.xselectionrequest.requestor;
			xev.selection = e.xselectionrequest.selection;
			xev.target = e.xselectionrequest.target;
			xev.property = e.xselectionrequest.property;
			xev.time = e.xselectionrequest.time;

			//Send message to requesting window that operation is done
			result = XSendEvent(d->dp, xev.requestor, 0, 0L,
					(XEvent *) &xev);
			if (result == BadValue || result == BadWindow)
				fprintf(stderr, "send SelectionRequest failed\n");

		} else {
			V2("%s unknown event %d\n", __func__, e.type);
		}
	}

	return 0;
}

static void
ts_xorg_client_driver_init(
		ts_display_p display)
{
	ts_xorg_client_p d = (ts_xorg_client_p)display;

	XSetErrorHandler(x11_error_handler);
	d->dp = XOpenDisplay(d->displayname);

	XA_CLIPBOARD = XInternAtom(d->dp, "CLIPBOARD", 0);

	d->root = DefaultRootWindow(d->dp);
	// Ignore any error here, this is "just in case"
    //int i = 1;
    //setsockopt (ConnectionNumber(d->dp), IPPROTO_TCP, TCP_NODELAY, &i, sizeof (i));

	V2("%s display %s : %p (%s)\n", __func__, d->displayname, d->dp, d->display.param);

	int screen = DefaultScreen(d->dp);
	display->bounds.w = DisplayWidth(d->dp, screen);
	display->bounds.h = DisplayHeight(d->dp, screen);

	V2("%s %s screen is %dx%d\n", __func__,
			display->name, display->bounds.w, display->bounds.h);

	ts_xorg_keymap_load(&d->map, d->dp);

	XTestGrabControl(d->dp, True);

	XSetWindowAttributes attr;
	attr.do_not_propagate_mask = 0;
	attr.cursor                = createBlankCursor(d);
	attr.override_redirect     = True;
	attr.event_mask = LeaveWindowMask;

	// create and return the window
	d->window = XCreateWindow(d->dp, d->root, 0, 0, 1, 1, 0, 0,
							InputOnly, CopyFromParent,
							CWDontPropagate | CWEventMask |
							CWOverrideRedirect | CWCursor,
							&attr);

	CARD16 level;
	BOOL state;
	DPMSInfo(d->dp, &level, &state);
	V3("%s %s is currently %s\n", __func__, display->name, state ? "OFF" : "ON");

	//DPMSEnable(d->dp);
	DPMSDisable(d->dp);

	//DPMSForceLevel(d->dp, /*activate ? DPMSModeStandby :*/ DPMSModeOn);

	d->remote.display = display;
	d->remote.mux = d->mux;
	d->remote.socket = ConnectionNumber(d->dp);
	d->remote.data_read = xorg_client_eventloop;
	ts_mux_register(&d->remote);

	if (display->param && display->master)
		ts_display_place(
				ts_master_get_main(display->master),
				display, display->param);
}

static void
ts_xorg_client_driver_run(
		ts_display_p display)
{
	while(1)
		sleep(1);
}

static void
ts_xorg_client_driver_mouse(
		struct ts_display_t *display,
		int dx, int dy)
{
	ts_xorg_client_p d = (ts_xorg_client_p)display;
    XTestFakeMotionEvent(
    		d->dp, 0,
    		display->mousex, display->mousey, CurrentTime);
	XFlush(d->dp);
}

static void
ts_xorg_client_driver_button(
		struct ts_display_t *display,
		int b, int down)
{
	ts_xorg_client_p d = (ts_xorg_client_p)display;

	XTestFakeButtonEvent(d->dp, b + 1, down, CurrentTime);
	XFlush(d->dp);
}

static void
ts_xorg_client_driver_wheel(
		struct ts_display_t *display,
		int wheel,
		int y, int x)
{
	ts_xorg_client_p d = (ts_xorg_client_p)display;

	int b = x < 0 ? 5 : 4;
	int n = x < 0 ? -x : x;
	n /= 32;
	if (!n) n++;

	V2("%s button %d count %d\n", __func__, b, n);
	for (int i = 0; i < n; i++) {
		XTestFakeButtonEvent(d->dp, b, 1, CurrentTime);
		XTestFakeButtonEvent(d->dp, b, 0, CurrentTime);
	}
	XFlush(d->dp);
}

static void
ts_xorg_client_driver_key(
		struct ts_display_t *display, uint16_t k, int down)
{
	ts_xorg_client_p d = (ts_xorg_client_p)display;
	if (k & 0xff00)
		k += 0x1000;
	V3("%s key 0x%04x %s ", __func__, k, down ? "down" : "up");
	k = ts_xorg_key_to_button(&d->map, k);
	V3(" mapped to button 0x%02x\n", k);

	XTestFakeKeyEvent(d->dp, k, down, CurrentTime);
	XFlush(d->dp);
}

static void
ts_xorg_client_driver_getclipboard(
		struct ts_display_t *display,
		struct ts_display_t *to)
{
	ts_xorg_client_p d = (ts_xorg_client_p)display;

	V3("%s\n", __func__);
	Display * dpy = d->dp;
	// Copy from application
//	Atom a1, a2;
	Atom type;
//	XSelectInput(dpy, w, StructureNotifyMask+ExposureMask);
	int format, result;
	unsigned long len, bytes_left;
	unsigned char *data;
	Window Sown;

	ts_clipboard_clear(&display->clipboard);

	XSelectInput(dpy, d->window, StructureNotifyMask+ExposureMask);

	Atom tries[] = { XA_PRIMARY /*, XA_CLIPBOARD*/, 0 };

	for (int i = 0; tries[i]; i++) {
		Sown = XGetSelectionOwner (dpy, tries[i]);
	//	printf ("Selection owner for try %d %i\n", i, (int)Sown);
		if (Sown == None)
			continue;
		if (Sown == d->window) {
			V2("%s We already own the selection, bailing\n", __func__);
			continue;
		}
		XConvertSelection (dpy, tries[i], XA_STRING, XA_PRIMARY,
				d->window, CurrentTime);
		XSync (dpy, 0);
		//
		// Do not get any data, see how much data is there
		//
		XGetWindowProperty (dpy, d->window,
				XA_PRIMARY, 0, 0,	  	  // offset - len
				0, 	 	  // Delete 0==FALSE
				XA_STRING,  // flag
			&type,		  // return type
			&format,	  // return format
			&len, &bytes_left,  //that
			&data);
//		printf ("type:%i len:%i format:%i byte_left:%i\n",
//			(int)type, (int)len, (int)format, (int)bytes_left);
		if (bytes_left > 0) {
			unsigned long none;
			result = XGetWindowProperty (dpy, d->window,
					XA_PRIMARY, 0, bytes_left,	  	  // offset - len
					0, 	 	  // Delete 0==FALSE
					XA_STRING,  // flag
				&type,		  // return type
				&format,	  // return format
				&len, &none,  //that
				&data);

			if (result == Success) {
				V3 ("%s DATA %d!! '%s'\n", __func__, (int)bytes_left, data);
				ts_clipboard_add(&display->clipboard, "text", bytes_left, (char*)data);
				break;
			}
			XFree (data);
		}
	}
	if (display->clipboard.flavorCount)
		ts_display_setclipboard(
				to,
				&display->clipboard);
}

static void
ts_xorg_client_driver_setclipboard(
		struct ts_display_t *display,
		ts_clipboard_p clipboard)
{
	if (!clipboard->flavorCount)
		return;

	ts_xorg_client_p d = (ts_xorg_client_p)display;

	for (int i = 0; i < clipboard->flavorCount; i++) {
		if (!strcmp(clipboard->flavor[i].name, "text")) {
			V3("%s adding %d bytes of %s\n", __func__,
					clipboard->flavor[i].size, clipboard->flavor[i].name);

			Atom tries[] = { XA_PRIMARY /*, XA_CLIPBOARD*/ , 0 };

			for (int i = 0; tries[i]; i++) {
				XChangeProperty(
						d->dp, d->window, tries[i], XA_STRING,
							8, PropModeReplace,
							(uint8_t*)clipboard->flavor[i].data,
							clipboard->flavor[i].size);

				//make this window own the clipboard selection
				XSetSelectionOwner(d->dp, tries[i], d->window, CurrentTime);
				if (d->window != XGetSelectionOwner(d->dp, tries[i]))
				  fprintf(stderr,"%s Could not set CLIPBOARD selection.\n", __func__);
			}
			XFlush(d->dp);

			return;
		}
	}
}

static void
ts_xorg_client_driver_enter(
		ts_display_p display)
{
//	ts_xorg_client_p d = (ts_xorg_client_p)display;
	ts_xorg_client_driver_mouse(display, 0, 0);
}

static void
ts_xorg_client_driver_leave(
		ts_display_p display)
{
//	ts_xorg_client_p d = (ts_xorg_client_p)display;
	ts_xorg_client_driver_mouse(display, 0, 0);
}


static ts_display_driver_t ts_xorg_client_driver = {
		.init = ts_xorg_client_driver_init,
		.run = ts_xorg_client_driver_run,
		.enter = ts_xorg_client_driver_enter,
		.leave = ts_xorg_client_driver_leave,
		.mouse = ts_xorg_client_driver_mouse,
		.button = ts_xorg_client_driver_button,
		.wheel = ts_xorg_client_driver_wheel,
		.key = ts_xorg_client_driver_key,
		.getclipboard = ts_xorg_client_driver_getclipboard,
		.setclipboard = ts_xorg_client_driver_setclipboard,
};

static ts_display_p
ts_xorg_client_main_create(
		ts_mux_p mux,
		ts_master_p master,
		char * param)
{
	ts_xorg_client_p res = malloc(sizeof(ts_xorg_client_t));
	memset(res, 0, sizeof(ts_xorg_client_t));

	res->mux = mux;

	char host[64];
	gethostname(host, sizeof(host));
	char * dot = strchr(host, '.');
	if (dot)
		*dot = 0;

	res->displayname = getenv("DISPLAY") ? getenv("DISPLAY") : ":0.0";

	ts_display_init(&res->display, master, &ts_xorg_client_driver, host, param);
	ts_master_display_add(master, &res->display);

	return &res->display;
}

static ts_display_p
ts_xorg_client_create(
		ts_mux_p mux,
		ts_master_p master,
		char * param)
{
	ts_xorg_client_p res = malloc(sizeof(ts_xorg_client_t));
	memset(res, 0, sizeof(ts_xorg_client_t));

	res->mux = mux;

	char name[128];
	strcpy(name, param);
	char * where = strchr(name, '=');
	if (where) {
		*where = 0;
		where++;
	}
	char * col = strchr(name, ':');
	if (col) *col = 0;

    struct hostent *hp = gethostbyname(name);
    if (!hp) {
    	fprintf(stderr, "%s host '%s' doesn't exists\n", __func__, name);
    	return NULL;
    }
    char displayname[128];
    sprintf(displayname, "%s:%s",
    		inet_ntoa(*(struct in_addr *) (hp->h_addr_list[0])),
    		col ? col+1 : "0.0");
	res->displayname = strdup(displayname);

	if (!isdigit(name[0])) {
		col = strchr(name, '.');
		if (col) * col = 0;
	}
	V2("%s name '%s' param '%s' display '%s'\n", __func__, name, param, displayname);

	int doproxy = 1;
	ts_display_driver_p driver = &ts_xorg_client_driver;
	if (doproxy) {
		driver = ts_display_proxy_driver(mux, &ts_xorg_client_driver);
	} else {
		// that will crash anyway, due to thread issues
		ts_xorg_client_driver.getclipboard = NULL;
		ts_xorg_client_driver.setclipboard = NULL;
	}
	ts_display_init(&res->display, master, driver, name, NULL);

	ts_master_display_add(master, &res->display);

	return &res->display;
}

extern ts_platform_create_callback_p ts_platform_create_client;
extern ts_platform_create_callback_p ts_xorg_create_client;

static void __xorg_init() __attribute__((constructor));

static void __xorg_init()
{
//	printf("%s\n",__func__);
	ts_platform_create_client = ts_xorg_client_main_create;
	ts_xorg_create_client = ts_xorg_client_create;
}

