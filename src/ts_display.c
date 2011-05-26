/*
	ts_display.c

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
#include "ts_defines.h"
#include "ts_verbose.h"

void
ts_display_init(
		ts_display_p d,
		struct ts_master_t * master,
		ts_display_driver_p driver,
		char * name,
		char * param )
{
	memset(d, 0, sizeof(*d));
	d->master = master;
	d->driver = driver;
	d->name = strdup(name);
	d->param = param ? strdup(param) : NULL;
	if (d->driver && d->driver->init)
		d->driver->init(d);
	V1("Creating display '%s' (%s)\n", name, __func__);
}

void
ts_display_dispose(
		ts_display_p d )
{
	if (d->name)
		free(d->name);
	if (d->param)
		free(d->param);
	d->param = NULL;
	d->name = NULL;
	d->master = NULL;
	if (d->driver && d->driver->dispose)
		d->driver->dispose(d);
}

ts_display_driver_p
ts_display_clone_driver(
		ts_display_driver_p driver )
{
	ts_display_driver_p res = malloc(sizeof(*driver));
	*res = *driver;
	res->_mutable = 1;
	return res;
}

void
ts_display_run(
		ts_display_p d )
{
	if (d && d->driver && d->driver->run)
		d->driver->run(d);
}

void
ts_display_enter(
		ts_display_p d )
{
	if (!d)
		return;
	d->moved = 0;
	d->mousex = d->master->mousex - d->bounds.x;
	d->mousey = d->master->mousey - d->bounds.y;
	V2("Entering display %s at %5d %5d (%s)\n", d->name, d->mousex, d->mousey, __func__);
	if (d->driver && d->driver->enter)
		d->driver->enter(d);
	d->active = 1;
}

void
ts_display_leave(
		ts_display_p d )
{
	if (!d)
		return;
	V3("Leaving display %s at %5d %5d\n", d->name, d->mousex, d->mousey, __func__);
	if (d && d->driver && d->driver->leave)
		d->driver->leave(d);
	d->active = 0;
}

int
ts_display_place(
		ts_display_p main,
		ts_display_p which,
		char * where )
{
	if (!which || !where)
		return -1;
	if (!main)
		main = ts_master_get_main(which->master);
	if (!main)
		return -1;
	//ts_rect_t m = main->bounds;

	V1("Placing %s %s of %s (%s)\n", which->name, where, main->name, __func__);
	if (!strcmp(where, "right")) {
		which->bounds.x = main->bounds.x + main->bounds.w;
		which->bounds.y = main->bounds.y;
	} else if (!strcmp(where, "top")) {
		which->bounds.x = main->bounds.x;
		which->bounds.y = main->bounds.y - which->bounds.h;
	} else if (!strcmp(where, "left")) {
		which->bounds.x = main->bounds.y - which->bounds.w;
		which->bounds.y = main->bounds.y;
	} else if (!strcmp(where, "bottom")) {
		which->bounds.x = main->bounds.x;
		which->bounds.y = main->bounds.y + main->bounds.h;
	} else {
		printf("%s unsupported place '%s'\n", __func__, where);
	}
	ts_rect_t w = which->bounds;
	V2("%s %s %d,%d %dx%d\n", __func__, which->name, w.x, w.y, w.w, w.h);
	return 0;
}


int
ts_display_movemouse(
		ts_display_p d,
		int dx, int dy )
{
	if (!d)
		return -1;
	if (dx || dy)
		d->moved = 1;
	d->mousex += dx;
	d->mousey += dy;
	if (d->driver && d->driver->mouse)
		d->driver->mouse(d, dx, dy);
	return 0;
}

void
ts_display_button(
		ts_display_p d,
		int button, int down )
{
	if (d && d->driver && d->driver->button)
		d->driver->button(d, button, down);
}

void
ts_display_wheel(
		ts_display_p d,
		int wheel, int x, int y )
{
	if (d && d->driver && d->driver->wheel)
		d->driver->wheel(d, wheel, x, y);
}

void
ts_display_key(
		ts_display_p d,
		uint16_t key, int down )
{
	if (d && d->driver && d->driver->key)
		d->driver->key(d, key, down);
}

void
ts_display_getclipboard(
		ts_display_p d,
		ts_display_p to)
{
	//printf("ts_display_getclipboard %p %s\n", d, d ? d->name : "");
	if (d && d->driver && d->driver->getclipboard)
		d->driver->getclipboard(d, to);
}

void
ts_display_setclipboard(
		ts_display_p d,
		ts_clipboard_p clipboard )
{
	//printf("ts_display_setclipboard %p %p %s driver %p setclipboard %p\n",
	//		d, clipboard, d ? d->name : "", d->driver,
	//				d->driver ? d->driver->setclipboard : NULL);
	if (d && d->driver && d->driver->setclipboard)
		d->driver->setclipboard(d, clipboard);
}

