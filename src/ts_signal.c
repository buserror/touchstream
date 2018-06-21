/*
 * ts_signal.c
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

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>

#include <netdb.h>
#include <unistd.h>
#include <stdio.h>

#include "ts_signal.h"


int
ts_signal_init(
		ts_signal_p res )
{
	if (socketpair(AF_UNIX, SOCK_DGRAM, 0, res->fd)) {
		perror("ts_signal_new socketpair");
		return -1;
	} else {
		for (int i = 0; i < 2; i++) {
			int flags = fcntl(res->fd[i], F_GETFL, 0);
			fcntl(res->fd[i], F_SETFL, flags | O_NONBLOCK);
		}
	}
	return 0;
}

void
ts_signal_dispose(
		ts_signal_p s)
{
	if (!s)
		return;
	close(s->fd[0]);
	close(s->fd[1]);
	s->fd[1] = s->fd[0] = -1;
}

//! Signal the one side
void
ts_signal(
		ts_signal_p s,
		int end,
		uint8_t what )
{
	if (!what) what++;
	if (write(s->fd[end], &what, 1))
		;
}

void
ts_signal_flush(
		ts_signal_p s,
		int end )
{
	uint8_t buf[32];
	while (read(s->fd[end], buf, sizeof(buf)) == sizeof(buf))
		;
}

//! Signal wait
uint8_t
ts_signal_wait(
		ts_signal_p s,
		int end,
		uint32_t inTimeout )
{
	fd_set set;
	FD_ZERO(&set);
	FD_SET(s->fd[end], &set);
	struct timeval tm = { .tv_sec = 0, .tv_usec = inTimeout * 1000 };
	int ret = select(s->fd[end]+1, &set, NULL, NULL, &tm);
	if (ret < 0) perror("vdmsg_funnel_wait_empty");
	if (ret > 0) {
		uint8_t byte;
		if (read(s->fd[end], &byte, 1) > 0) {
			return byte;
		}
	}
	return 0;
}
