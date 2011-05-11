/*
	ts_xorg.h

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


#ifndef __TS_XORG_H___
#define __TS_XORG_H___

typedef struct ts_xorg_krev_t {
	struct {
		int count;
		struct {
			unsigned long key : 16, code : 8;
		} v[4];
	} map[255];
} ts_xorg_krev_t, * ts_xorg_krev_p;

int
ts_xorg_keymap_load(
		ts_xorg_krev_p map,
		Display * m_display);

int
ts_xorg_key_to_button(
		ts_xorg_krev_p map,
		uint32_t key);

#endif /* __TS_XORG_H___ */
