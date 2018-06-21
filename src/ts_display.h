/*
	sh_display.h

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
 * a "display" is what describes a screen. It is mostly just a rectangle, and a
 * 'local' (zero based) mouse position.
 *
 * The "active" display is the one that gets the events sent to itself, all the
 * other ones don't get them.
 *
 * Associated with the display is a "driver" -- a list of callbacks that are called
 * for each of the events that display can handle, not all callbacks are needed
 *
 * There are currently a few "drivers"
 * + OSX "master" one, mostly cares about enter/leave and the clipbaord calls,
 *   the 'user input' ones aren't needed since it's a master screen
 * + xorg "slave" one, this one implements all the callback, and sends the event
 *   received to the target Xorg connection using XTest etc.
 * + A "proxy" one, needed because the OSX one can't call the "xorg" directly, since
 *   they live on different threads. Instead, the proxy queues the events and
 *   deliver them to the "slave" driver on the mux's (xorg) thread.
 * + A "fake" one, that is used my the mux "data" remote to recover the clipboard
 *   from a remote client screen and send it back to the server
 *
 * A display also has a "clipboard", a primitive structure where it can store
 * "flavors" of data. See ts_clipboard.[ch]
 */
#ifndef __SH_DISPLAY_H___
#define __SH_DISPLAY_H___

#include <stdint.h>

#include "ts_clipboard.h"

typedef struct ts_rect_t {
	int16_t x, y, w, h;
} ts_rect_t, *ts_rect_p;

struct ts_display_t;
struct ts_master_t;

typedef struct ts_display_driver_t {
	unsigned int _mutable : 1;	// can be free()ed ?
	void * refCon;	// reference constant, optional, used by callbacks
	void (*init)(struct ts_display_t *d);
	void (*dispose)(struct ts_display_t *d);
	void (*run)(struct ts_display_t *d);

	void (*enter)(struct ts_display_t *d);
	void (*leave)(struct ts_display_t *d);

	void (*mouse)(struct ts_display_t *d, int dx, int dy);
	void (*button)(struct ts_display_t *d, int b, int down);
	void (*key)(struct ts_display_t *d, uint16_t k, int down);
	void (*wheel)(struct ts_display_t *d, int wheel, int y, int x);

	void (*getclipboard)(struct ts_display_t *d, struct ts_display_t *to);
	void (*setclipboard)(struct ts_display_t *d, ts_clipboard_p clipboard);
} ts_display_driver_t, *ts_display_driver_p;


typedef struct ts_display_t {
	struct ts_master_t * master;
	char * name;
	char * param;
	ts_display_driver_p	driver;

	ts_rect_t bounds;
	unsigned int active : 1, moved : 1;

	int mousex, mousey;

	ts_clipboard_t clipboard;
} ts_display_t, *ts_display_p;

/*
 * Initializes display 'd'. Sets it's master to 'master' BUT does NOT
 * add it to the master's list just yet.
 * 'driver' is a set of (optional) callbacks to call when this display
 * becomes active,
 * the 'name' is just a handle to allow referal, it is typicaly the
 * hostname of the display is on.
 * 'param' is a parameter string that is currently used for ts_display_place()
 */
void
ts_display_init(
		ts_display_p d,
		struct ts_master_t * master,
		ts_display_driver_p driver,
		char * name,
		char * param );
void
ts_display_dispose(
		ts_display_p d );

/*
 * moves 'which' origin to be 'where' relative to 'main'.
 * if 'main' is NULL, the master's main disolay is used.
 *
 * 'where' can be 'right', 'top', 'left', 'bottom' for now,
 * TODO finish extra placement flags
 */
int
ts_display_place(
		ts_display_p main,
		ts_display_p which,
		char * where );

/*
 * returns a mutable copy of a driver
 */
ts_display_driver_p
ts_display_clone_driver(
		ts_display_driver_p driver );

/*
 * The following calls try to call down to the driver to do the job
 */
void
ts_display_run(
		ts_display_p d );

void
ts_display_enter(
		ts_display_p d);
void
ts_display_leave(
		ts_display_p d );
int
ts_display_movemouse(
		ts_display_p d,
		int dx, int dy );
void
ts_display_button(
		ts_display_p d,
		int button, int down );
void
ts_display_wheel(
		ts_display_p d,
		int button, int x, int y );
void
ts_display_key(
		ts_display_p d,
		uint16_t key, int down );
void
ts_display_getclipboard(
		ts_display_p d,
		ts_display_p to);
void
ts_display_setclipboard(
		ts_display_p d,
		ts_clipboard_p clipboard );


/*
 * Rectangle utilities
 */

// Return != 0 if x,y is in 'r'
static inline int
ts_ptinrect(
		ts_rect_p r,
		int x, int y)
{
	return x >= r->x && x < r->x + r->w &&
			y >= r->y && y < r->y + r->h;
}

// forces x,y to be inside 'r'
static inline void
ts_clippt(
		ts_rect_p r,
		int *x, int *y)
{
	if (*x < r->x)	*x = r->x;
	if (*x >= r->x + r->w)	*x = r->x + r->w - 1;
	if (*y < r->y)	*y = r->y;
	if (*y >= r->y + r->h)	*y = r->y + r->h - 1;
}

// return != 0 if x,h is on an edge of 'r'
static inline int
ts_ptonedge(
		ts_rect_p r,
		int x, int y)
{
	return 	x == r->x || y == r->y ||
			x == (r->x + r->w - 1) || (y == r->y + r->h - 1);
}

// shift x,y by one outside of 'r' if they are on an edge
static inline void
ts_pt_out_edge(
		ts_rect_p r,
		int *x, int *y)
{
	if (*x == r->x) *x -= 1;
	if (*y == r->y) *y -= 1;
	if (*x == r->x + r->w - 1)	*x += 1;
	if (*y == r->y + r->h - 1)	*y += 1;
}

#endif /* __SH_DISPLAY_H___ */
