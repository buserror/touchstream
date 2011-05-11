/*
	ts_mux.h

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
 * a "mux" is the keystone of the whole system, allowing to serialize
 * all the events for the displays into one single thread, thus preventing
 * the need for mutexes etc.
 * The 'mux' uses 'remote' small structure that are associated with a socket,
 * and can also have as many callbacks to customize it's behaviour.
 *
 * There are a few types of "remotes"
 * + The "listen" remote is just a listen socket waiting for incoming
 *   connections, when one is received, it creates a "data" remote with it
 * + A "connect" remote is an outgoing connection, it will behave mostly
 *   as a "data" socket when it has been established
 * + A "xorg" remote is what is used by the xorg "display" subsystem to
 *   serialize the reception of events via XPending(). See ts_xorg_client.[ch]
 * + A "proxy" remote is defined in ts_display_proxy.[ch], it it's just
 *   a signaling socket that allows that proxy to empty it's fifo and make
 *   it's calls to a "slace" display in the mux's thread.
 */
#ifndef __TS_MUX_H___
#define __TS_MUX_H___

#include <netinet/in.h>
#include "ts_display.h"
#include "ts_master.h"
#include "ts_signal.h"


enum {
	skt_state_None = 0,
	skt_state_Listen,
	skt_state_Connect,
	skt_state_Data,
};

struct ts_display_proxy_driver_t;
/*
 * a ts_remote_t handles one connection for the mux. They can be
 * listen remotes, data (accepted) remotes, connect (outgoing)
 * remotes etc
 */
struct ts_mux_t;
typedef struct ts_remote_t {
	struct ts_mux_t * mux;
	ts_display_p display;
	int socket;
	int state;
	struct sockaddr_in  addr;
	int accept_socket;
	time_t timeout;

	struct ts_display_proxy_driver_t * proxy;

	int		in_len;
	int		in_size;
	uint8_t * in;

	int 	out_len;
	int 	out_size;
	uint8_t * out;

	int (*start)(struct ts_remote_t * remote);
	int (*restart)(struct ts_remote_t * remote);
	int (*dispose)(struct ts_remote_t * remote);

	int (*can_read)(struct ts_remote_t * remote);
	int (*data_read)(struct ts_remote_t * remote);
	int (*can_write)(struct ts_remote_t * remote);
	int (*data_write)(struct ts_remote_t * remote);

} ts_remote_t, *ts_remote_p;

/*
 * A mux handles up to 32 remote
 */
typedef struct ts_mux_t {
	ts_master_p master;
	pthread_t	thread;

	ts_signal_t signal;
	uint32_t dp_usage;
	ts_remote_p dp[32];
} ts_mux_t, *ts_mux_p;

/*
 * This is called by ts_mux_port_new()
 */
int
ts_mux_start(
		ts_mux_p mux,
		ts_master_p master );

int
ts_mux_port_new(
		ts_mux_p mux,
		ts_master_p master,
		char * address,
		ts_display_p display);

void
ts_mux_signal(
		ts_mux_p mux,
		uint8_t what );

int
ts_mux_register(
		ts_remote_p r );
int
ts_mux_unregister(
		ts_remote_p r);

#endif /* __TS_MUX_H___ */
