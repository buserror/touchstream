/*
	ts_defines.h

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


#ifndef __TS_DEFINES_H___
#define __TS_DEFINES_H___

#include "ts_display.h"
#include "ts_master.h"

struct ts_mux_t;
typedef struct ts_display_t *
	(*ts_platform_create_callback_p)(
		struct ts_mux_t * mux,
		struct ts_master_t * master,
		char * param);

#endif /* __TS_DEFINES_H___ */
