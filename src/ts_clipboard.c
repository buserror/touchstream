/*
	ts_clipboard.c

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
#include <string.h>
#include <stdio.h>
#include "ts_clipboard.h"

void
ts_clipboard_clear(
		ts_clipboard_p clip )
{
	for (int i = 0; i < clip->flavorCount; i++) {
		if (clip->flavor[i].name)
			free(clip->flavor[i].name);
		if (clip->flavor[i].data)
			free(clip->flavor[i].data);
		clip->flavor[i].name = NULL;
		clip->flavor[i].data = NULL;
		clip->flavor[i].size = 0;
	}
	clip->flavorCount = 0;
}

int
ts_clipboard_add(
		ts_clipboard_p clip,
		char * flavor,
		int size,
		char * data )
{
	int slot = -1;
	for (int i = 0; i < clip->flavorCount; i++)
		if (!strcmp(clip->flavor[i].name, flavor)) {
			slot = i;
			break;
		}
	if (slot == -1) {
		if (clip->flavorCount == (sizeof(clip->flavor) / sizeof(clip->flavor[0])))
			return -1;
		slot = clip->flavorCount++;
		clip->flavor[slot].name = strdup(flavor);
	}
	clip->flavor[slot].data =  realloc(
			clip->flavor[slot].data,
			clip->flavor[slot].size + size + 1);
	memcpy(clip->flavor[slot].data + clip->flavor[slot].size,
			data, size);
	clip->flavor[slot].size += size;
	clip->flavor[slot].data[clip->flavor[slot].size] = 0;
	return 0;
}
