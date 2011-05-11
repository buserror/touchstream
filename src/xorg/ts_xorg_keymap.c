/*
	ts_xorg_keymap.c

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


#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "ts_defines.h"
#include "ts_keymap.h"

#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>

#include "ts_xorg.h"

int
ts_xorg_keymap_load(
		ts_xorg_krev_p map,
		Display * m_display)
{
	// get the number of keycodes
	int minKeycode, maxKeycode;
	XDisplayKeycodes(m_display, &minKeycode, &maxKeycode);
	int numKeycodes = maxKeycode - minKeycode + 1;

//	printf("xorg_load_keymap min %04x max %04x\n", minKeycode, maxKeycode);

	// get the keyboard mapping for all keys
	int keysymsPerKeycode;
	KeySym* allKeysyms = XGetKeyboardMapping(m_display,
								minKeycode, numKeycodes,
								&keysymsPerKeycode);

//	printf("xorg_load_keymap min %p keysymsPerKeycode %d\n", allKeysyms, keysymsPerKeycode);

	for (int i = 0; i < numKeycodes; i++) {
//		printf("code %04x : ", minKeycode + i);
		KeySym * s = allKeysyms + (i * keysymsPerKeycode);
		int k = 0;
		if (!s[k])
			continue;
		//printf("%08x ", (int)s[k]);
		int b = s[k] & 0xff;
		if (map->map[b].count == 4) {
			printf("%s overflow map %02x\n", __func__, (int)s[k] & 0xff);
		} else {
			map->map[b].v[map->map[b].count].code = minKeycode + i;
			map->map[b].v[map->map[b].count].key = s[k];
			map->map[b].count++;
		}

		//printf("\n");
	}
	XFree(allKeysyms);
	return 0;
}

int
ts_xorg_key_to_button(
		ts_xorg_krev_p map,
		uint32_t key)
{
	int b = key & 0xff;
	for (int i = 0; i < map->map[key & 0xff].count; i++)
		if (map->map[b].v[i].key == key)  {
		//	printf("%s mapped %04x to button %02x\n",__func__, key, map->map[b].v[i].code);
			return map->map[b].v[i].code;
		}
	return key;
}
