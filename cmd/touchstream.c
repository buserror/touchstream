/*
	touchstream.c

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
#include <netdb.h>
#include <libgen.h>
#include <stdarg.h>
#include <ctype.h>

#include "ts_defines.h"
#include "ts_mux.h"
#include "ts_verbose.h"

int verbose = 0;

void V1(const char * format, ...)
{
	if (verbose < 1) return;
	va_list va;
	va_start(va, format);
	vprintf(format, va);
	va_end(va);
}
void V2(const char * format, ...)
{
	if (verbose < 2) return;
	va_list va;
	va_start(va, format);
	vprintf(format, va);
	va_end(va);
}
void V3(const char * format, ...)
{
	if (verbose < 3) return;
	va_list va;
	va_start(va, format);
	vprintf(format, va);
	va_end(va);
}

/*
 * These globals are "special", they are exported, and
 * are set by platform specific code in a __attribute__((constructor))
 * function.
 */
ts_platform_create_callback_p ts_platform_create_server = NULL;
ts_platform_create_callback_p ts_platform_create_client = NULL;
ts_platform_create_callback_p ts_xorg_create_client = NULL;

ts_mux_t mux[1];
ts_master_t master[1];

int
main(
		int argc,
		char * argv[])
{
	int server = 1;
	char * client = NULL;
	char * param = NULL;
	char * xorg[8] = {0};
	int xorgCount = 0;

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-s")) {
			server++;
		} else if (!strncmp(argv[i], "-v", 2)) {
			if (isdigit(argv[i][2]))
				verbose = isdigit(argv[i][2]) - '0';
			else
				verbose++;
		} else if (!strcmp(argv[i], "-x") && i < argc-1) {
			if (!ts_xorg_create_client) {
				fprintf(stderr, "%s: xorg client mode unsupported on this platform\n",
						basename(argv[0]));
				exit(1);
			}
			xorg[xorgCount++] = argv[++i];
		} else if (!strcmp(argv[i], "-c") && i < argc-1) {
			i++;
			param = argv[i];
			char * name = strsep(&param, "=");
			char * col = strchr(name, ':');
			if (col) *col = 0;
			if (gethostbyname(name)) {
				client = argv[i];	// with :<port>
				server = 0;
				V1("%s client host: '%s'\n", argv[0], name);
			}
		}
	}
	ts_master_init(master);

	ts_platform_create_callback_p platform = NULL;

	if (server) {
		if (ts_platform_create_server)
			platform = ts_platform_create_server;
		else {
			fprintf(stderr, "%s: server mode unsupported on this platform\n",
					basename(argv[0]));
			exit(1);
		}
	} else {
		if (ts_platform_create_client)
			platform = ts_platform_create_client;
		else {
			fprintf(stderr, "%s: client mode unsupported on this platform\n",
					basename(argv[0]));
			exit(1);
		}
	}
	ts_display_p main_display = platform(mux, master, param);

	ts_mux_port_new(mux, master, client, main_display);

	for (int i = 0; i < xorgCount; i++)
		ts_xorg_create_client(mux, master, xorg[i]);

	ts_display_run(main_display);
}
