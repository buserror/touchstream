/*
	ts_master.h

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
 * The "master" structure holds a list of "display", and holds the "main"
 * mouse cursor, when the mouse is moved, it detects any boundary changes
 * and "activates" (ie "enter") the new active one while "leaving" the
 * previously active one.
 *
 * There is a convention that the first display in the list is the "main" one,
 * regarless of wether you are a server or a client.
 */

#ifndef __TS_MASTER_H___
#define __TS_MASTER_H___

#include "ts_display.h"

typedef struct ts_master_t {
	int displayCount;
	ts_display_p display[8];
	ts_display_p active;

	int mousex, mousey;
} ts_master_t, *ts_master_p;

void
ts_master_init(
		ts_master_p master);

void
ts_master_display_add(
		ts_master_p master,
		ts_display_p d );

void
ts_master_display_remove(
		ts_master_p master,
		ts_display_p d);

ts_display_p
ts_master_display_get(
		ts_master_p master,
		char * display);

ts_display_p
ts_master_display_get_for(
		ts_master_p master,
		int x, int y);

ts_display_p
ts_master_get_active(
		ts_master_p master);

ts_display_p
ts_master_get_main(
		ts_master_p master);

int
ts_master_set_active(
		ts_master_p master,
		ts_display_p d );

void
ts_master_mouse_move(
		ts_master_p m,
		int dx, int dy );

#endif /* __TS_MASTER_H___ */
