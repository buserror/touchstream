/*
	ts_display_proxy.c

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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "ts_display_proxy.h"
#include "ts_mux.h"

DEFINE_FIFO(ts_display_proxy_event_t, proxy_fifo);

static int
ts_proxy_driver_flush(
		struct ts_remote_t * r)
{
	{
		uint8_t buf[32];
		while (read(r->socket, buf, sizeof(buf)) == sizeof(buf))
			;
	}

//	printf("%s display %p\n", __func__, r->display);
	if (!r->display)
		return 0;

	ts_display_p display = r->display;
	ts_display_proxy_driver_p d = (ts_display_proxy_driver_p)display->driver;

	while (!proxy_fifo_isempty(&d->fifo)) {
		ts_display_proxy_event_t e = proxy_fifo_read(&d->fifo);
		switch (e.event) {
			case ts_proxy_init:
				d->slave->init(display);
				break;
			case ts_proxy_dispose:
				d->slave->dispose(display);
				break;
			case ts_proxy_enter:
				d->slave->enter(display);
				break;
			case ts_proxy_leave:
				d->slave->leave(display);
				break;
			case ts_proxy_mouse:
				d->slave->mouse(display, e.u.mouse.x,  e.u.mouse.y);
				break;
			case ts_proxy_button:
				d->slave->button(display, e.u.button, e.down);
				break;
			case ts_proxy_key:
				d->slave->key(display, e.u.key, e.down);
				break;
			case ts_proxy_wheel:
				d->slave->wheel(display, e.u.wheel.wheel, e.u.wheel.y, e.u.wheel.x);
				break;
			case ts_proxy_getclipboard:
				d->slave->getclipboard(display, e.u.display);
				break;
			case ts_proxy_setclipboard:
				d->slave->setclipboard(display, e.u.clipboard);
				break;
		}
	}
	return 0;
}

static void
ts_proxy_driver_init(
		ts_display_p d)
{
	ts_display_proxy_driver_p p = (ts_display_proxy_driver_p)d->driver;
	p->remote.display = d;
	if (p->slave && !p->slave->init)
		return;
	ts_display_proxy_event_t e = {
			.event = ts_proxy_init,
	};
	proxy_fifo_write(&p->fifo, e);
	ts_signal(&p->signal, TS_SIGNAL_END0, 0);
}

static void
ts_proxy_driver_dispose(
		ts_display_p d)
{
	ts_display_proxy_driver_p p = (ts_display_proxy_driver_p)d->driver;
	if (p->slave && !p->slave->dispose)
		return;
	ts_display_proxy_event_t e = {
			.event = ts_proxy_dispose,
	};
	proxy_fifo_write(&p->fifo, e);
	ts_signal(&p->signal, TS_SIGNAL_END0, 0);
}

static void
ts_proxy_driver_enter(
		ts_display_p d)
{
	ts_display_proxy_driver_p p = (ts_display_proxy_driver_p)d->driver;
	if (p->slave && !p->slave->enter)
		return;
	ts_display_proxy_event_t e = {
			.event = ts_proxy_enter,
	};
	proxy_fifo_write(&p->fifo, e);
	ts_signal(&p->signal, TS_SIGNAL_END0, 0);
}

static void
ts_proxy_driver_leave(
		ts_display_p d)
{
	ts_display_proxy_driver_p p = (ts_display_proxy_driver_p)d->driver;
	if (p->slave && !p->slave->leave)
		return;
	ts_display_proxy_event_t e = {
			.event = ts_proxy_leave,
	};
	proxy_fifo_write(&p->fifo, e);
	ts_signal(&p->signal, TS_SIGNAL_END0, 0);
}

static void
ts_proxy_driver_mouse(
		ts_display_p d,
		int x, int y)
{
	ts_display_proxy_driver_p p = (ts_display_proxy_driver_p)d->driver;
	if (p->slave && !p->slave->mouse)
		return;
	ts_display_proxy_event_t e = {
			.event = ts_proxy_mouse,
			.u.mouse.x = x,
			.u.mouse.y = y,
	};
	proxy_fifo_write(&p->fifo, e);
	ts_signal(&p->signal, TS_SIGNAL_END0, 0);
}

static void
ts_proxy_driver_button(
		ts_display_p d,
		int b,
		int down)
{
	ts_display_proxy_driver_p p = (ts_display_proxy_driver_p)d->driver;
	if (p->slave && !p->slave->button)
		return;
	ts_display_proxy_event_t e = {
			.event = ts_proxy_button,
			.u.button = b,
			.down = down ? 1 : 0,
	};
	proxy_fifo_write(&p->fifo, e);
	ts_signal(&p->signal, TS_SIGNAL_END0, 0);
}

static void
ts_proxy_driver_key(
		ts_display_p d,
		uint16_t k,
		int down)
{
	ts_display_proxy_driver_p p = (ts_display_proxy_driver_p)d->driver;
	if (p->slave && !p->slave->key)
		return;
	ts_display_proxy_event_t e = {
			.event = ts_proxy_key,
			.u.key = k,
			.down = down ? 1 : 0,
	};
	proxy_fifo_write(&p->fifo, e);
	ts_signal(&p->signal, TS_SIGNAL_END0, 0);
}

static void
ts_proxy_driver_wheel(
		ts_display_p d,
		int wheel,
		int y, int x)
{
	ts_display_proxy_driver_p p = (ts_display_proxy_driver_p)d->driver;
	if (p->slave && !p->slave->wheel)
		return;
	ts_display_proxy_event_t e = {
			.event = ts_proxy_wheel,
			.u.wheel.wheel = wheel,
			.u.wheel.y = y,
			.u.wheel.x = x,
	};
	proxy_fifo_write(&p->fifo, e);
	ts_signal(&p->signal, TS_SIGNAL_END0, 0);
}

static void
ts_proxy_driver_getclipboard(
		ts_display_p d,
		ts_display_p to)
{
	ts_display_proxy_driver_p p = (ts_display_proxy_driver_p)d->driver;
	if (p->slave && !p->slave->getclipboard)
		return;
	ts_display_proxy_event_t e = {
			.event = ts_proxy_getclipboard,
			.u.display = to,
	};
	proxy_fifo_write(&p->fifo, e);
	ts_signal(&p->signal, TS_SIGNAL_END0, 0);
}

static void
ts_proxy_driver_setclipboard(
		ts_display_p d,
		ts_clipboard_p clipboard)
{
	ts_display_proxy_driver_p p = (ts_display_proxy_driver_p)d->driver;
	//printf("ts_proxy_driver_setclipboard\n");
	if (p->slave && !p->slave->setclipboard)
		return;
	ts_display_proxy_event_t e = {
			.event = ts_proxy_setclipboard,
			.u.clipboard = clipboard,
	};
	proxy_fifo_write(&p->fifo, e);
	ts_signal(&p->signal, TS_SIGNAL_END0, 0);
}

static ts_display_driver_t ts_proxy_driver = {
		.init = ts_proxy_driver_init,
		.dispose = ts_proxy_driver_dispose,
		.enter = ts_proxy_driver_enter,
		.leave = ts_proxy_driver_leave,
		.mouse = ts_proxy_driver_mouse,
		.button = ts_proxy_driver_button,
		.key = ts_proxy_driver_key,
		.wheel = ts_proxy_driver_wheel,
		.getclipboard = ts_proxy_driver_getclipboard,
		.setclipboard = ts_proxy_driver_setclipboard,
};

ts_display_driver_p
ts_display_proxy_driver(
		ts_mux_p mux,
		ts_display_driver_p driver )
{
	ts_display_proxy_driver_p res = malloc(sizeof(ts_display_proxy_driver_t));
	memset(res, 0, sizeof(*res));
	res->driver = ts_proxy_driver;

	if (driver) {
		res->slave = ts_display_clone_driver(driver);
		ts_signal_init(&res->signal);

		res->remote.display = NULL;
		res->remote.mux = mux;
		res->remote.socket = res->signal.fd[TS_SIGNAL_END1];
		res->remote.data_read = ts_proxy_driver_flush;
		ts_mux_register(&res->remote);
	}
	return &res->driver;
}
