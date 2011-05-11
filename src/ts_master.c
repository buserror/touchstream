/*
	ts_master.c

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


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ts_defines.h"
#include "ts_verbose.h"

void
ts_master_init(
		ts_master_p master)
{
	memset(master, 0, sizeof(*master));
}

void
ts_master_display_add(
		ts_master_p master,
		ts_display_p d)
{
	for (int i = 0; i < master->displayCount; i++)
		if (master->display[i] == d)
			return;

	master->display[master->displayCount++] = d;
	d->master = master;
	if (master->displayCount == 1)
		ts_master_set_active(master, d);
}

void
ts_master_display_remove(
		ts_master_p master,
		ts_display_p d)
{
	for (int i = 0; i < master->displayCount && d; i++)
		if (master->display[i] == d) {
			memmove(master->display + i,
					master->display + i + 1,
					(master->displayCount - i - 1) * sizeof(ts_display_p));
			master->displayCount--;
			if (master->active == d) {
				if (master->displayCount && d != master->display[0])
					ts_master_set_active(master, master->display[0]);
				else
					ts_master_set_active(master, NULL);
			}
			ts_display_dispose(d);
			break;
		}
}

ts_display_p
ts_master_display_get(
	ts_master_p master,
	char * display )
{
	if (!master)
		return NULL;
	for (int i = 0; i < master->displayCount; i++)
		if (!strcmp(master->display[i]->name, display))
			return master->display[i];
	return NULL;
}

ts_display_p
ts_master_display_get_for(
		ts_master_p master,
		int x, int y)
{
	// first check the 'active' screen, it's probably a match anyway,
	// then check the main one, since it's very like to be that if not,
	// lastly, check all the others
	if (ts_ptinrect(&master->active->bounds, x, y))
		return master->active;
	else if (master->active != master->display[0] &&
			ts_ptinrect(&master->display[0]->bounds, x, y))
		return master->display[0];
	else {
		for (int i = 1 /* we did zero */; i < master->displayCount; i++)
			if (ts_ptinrect(&master->display[i]->bounds, x, y))
				return master->display[i];
	}
	return NULL;
}

ts_display_p
ts_master_get_active(
	ts_master_p master )
{
	return master->active;
}

ts_display_p
ts_master_get_main(
	ts_master_p master )
{
	return master->displayCount ? master->display[0] : NULL;
}


int
ts_master_set_active(
	ts_master_p master,
	ts_display_p d )
{
	if (!master)
		return -1;

	if (master->active == d)
		return 0;
	ts_display_p old = master->active;
	if (old)
		ts_display_leave(old);
	master->active = d ? d : master->displayCount ? master->display[0] : NULL;
	if (master->active) {
		V2("%s %s\n", __func__, master->active->name);
		ts_display_enter(master->active);
		// get clipboard is asynchronous, it's it's job to decide
		// to set the clipboard when it eventualy gets one
		if (old)
			ts_display_getclipboard(old, master->active);
	}
	return 0;
}

void
ts_master_mouse_move(
		ts_master_p m,
		int dx, int dy )
{
	int wasedge = ts_ptonedge(&m->active->bounds, m->mousex, m->mousey);
	int nx = m->mousex + dx;
	int ny = m->mousey + dy;

	ts_display_p newd = ts_master_display_get_for(m, nx, ny);
	/*
	 * If we're outside of any screen, clip the coordinates to the active one
	 */
	if (!newd || !newd->moved)
		newd = m->active;

	/*
	 * If we are on the active screen, watch for the mouse arriving on an edge
	 */
	if (newd == m->active) {
		int isedge = ts_ptonedge(&m->active->bounds, nx, ny);
		if (!wasedge && isedge) {
			V2("%s edge detected\n", __func__);
			// offset the coordinates to see if there is a screen 'over' the edge
			ts_pt_out_edge(&m->active->bounds, &nx, &ny);
			ts_display_p stick = ts_master_display_get_for(m, nx, ny);
			if (stick)
				newd = stick;
		}
	}

	ts_clippt(&newd->bounds, &nx, &ny);
	dx = nx - m->mousex;
	dy = ny - m->mousey;
	m->mousex = nx;
	m->mousey = ny;

//	V3("%s %5d %5d\n", __func__, m->mousex, m->mousey);

	// move the mouse before switching target, since enter() resets the mouse
	ts_display_movemouse(m->active, dx, dy);
	if (newd != m->active)
		ts_master_set_active(m, newd);

}
