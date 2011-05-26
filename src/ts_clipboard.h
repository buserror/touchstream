/*
	ts_clipboard.h

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
 * This small structure is made to store various "flavors" of clipboard
 * data. Right now only strings are supported, but pretty much anything
 * can be stored in there...
 */
#ifndef __TS_CLIPBOARD_H___
#define __TS_CLIPBOARD_H___

#include <sys/types.h>
#include <stdint.h>

typedef struct ts_clipboard_t {
	int flavorCount;
	struct {
		char * name;
		size_t size;
		uint8_t * data;
	} flavor[8];
} ts_clipboard_t, *ts_clipboard_p;

void
ts_clipboard_clear(
		ts_clipboard_p clip );

int
ts_clipboard_add(
		ts_clipboard_p clip,
		char * flavor,
		uint8_t * data,
		size_t size );


#endif /* __TS_CLIPBOARD_H___ */
