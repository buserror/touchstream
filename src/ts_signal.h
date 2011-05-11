/*
 * ts_signal.h
 *
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
 * Small utility that uses a socketpair as a semaphore that can be
 * used with select()
 */
#ifndef TS_SIGNAL_H_
#define TS_SIGNAL_H_

#include <stdint.h>

enum {
	TS_SIGNAL_END0 = 0,
	TS_SIGNAL_END1
};

typedef struct ts_signal_t {
	int fd[2];
} ts_signal_t, *ts_signal_p;

int
ts_signal_init(
		ts_signal_p s);

void
ts_signal_dispose(
		ts_signal_p s);

//! Signal the one side
void
ts_signal(
		ts_signal_p s,
		int end,
		uint8_t what );

void
ts_signal_flush(
		ts_signal_p s,
		int end );

//! Signal wait
uint8_t
ts_signal_wait(
		ts_signal_p s,
		int end,
		uint32_t inTimeout /* in milliseconds */ );

#endif /* VD_SIGNAL_H_ */
