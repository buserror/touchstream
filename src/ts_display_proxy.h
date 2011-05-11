/*
	ts_display_proxy.h

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


#ifndef __TS_DISPLAY_PROXY_H___
#define __TS_DISPLAY_PROXY_H___

#include "ts_display.h"
#include "ts_signal.h"
#include "ts_mux.h"

enum {
	ts_proxy_init = 0,
	ts_proxy_dispose,
	ts_proxy_enter,
	ts_proxy_leave,
	ts_proxy_mouse,
	ts_proxy_button,
	ts_proxy_key,
	ts_proxy_wheel,
	ts_proxy_getclipboard,
	ts_proxy_setclipboard,
};

typedef struct ts_display_proxy_event_t {
	uint32_t event : 8, flags : 8, down : 1;
	union {
		ts_display_p display;
		struct {
			 long x : 12, y : 12;
		} mouse;
		uint8_t button;
		uint16_t key;
		struct {
			long wheel : 4, y : 14, x : 14;
		} wheel;
		ts_clipboard_p clipboard;
	} u;
} ts_display_proxy_event_t, *ts_display_proxy_event_p;

#include "fifo_declare.h"

DECLARE_FIFO(ts_display_proxy_event_t, proxy_fifo, 32);

typedef struct ts_display_proxy_driver_t {
	ts_display_driver_t driver;
	ts_display_driver_p slave;

	ts_signal_t signal;
	proxy_fifo_t fifo;
	ts_remote_t remote;
} ts_display_proxy_driver_t, *ts_display_proxy_driver_p;


ts_display_driver_p
ts_display_proxy_driver(
		ts_mux_p  mux,
		ts_display_driver_p driver );

#endif /* __TS_DISPLAY_PROXY_H___ */
