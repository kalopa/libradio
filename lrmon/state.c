/*
 * Copyright (c) 2024, Kalopa Robotics Limited.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ABSTRACT
 * This file contains the state machine for the daemon. The idea here is to
 * manage and maintain the relationship state between this code and the
 * low-level board. What we don't want to do is assume the low-level board
 * isn't running or that it needs to be re-initialized. So we only reset the
 * low-level board if we really think it is dead.
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <syslog.h>

#include "lrmon.h"
#include "libradio.h"

#define STATE_UNKNOWN			0
#define STATE_SREQUEST			1
#define STATE_SEND_RESET		2
#define STATE_DREQUEST			3
#define STATE_ACTIVATE			4
#define STATE_SET_TIME			5
#define STATE_SET_DATE			6
#define STATE_ACTIVATE_CH0		7
#define STATE_ACTIVATE_CH1		8
#define STATE_ACTIVATE_CH2		9
#define STATE_ACTIVATE_CH3		10
#define STATE_READY				11

int 	state;
int		retries;

void	dynamic_status_timer();

/*
 *
 */
void
state_init()
{
	state = STATE_UNKNOWN;
	retries = 0;
	timer_insert(state_machine, 3);
}

/*
 * We haven't heard anything from the board. Ask for a status.
 */
void
state_machine()
{
	int i, chstate, next_timeout = 600;

	syslog(LOG_DEBUG, "State machine %d (rr%d, retries %d)\n", state, response_received, retries);
	timer_remove(state_machine);
	if (state == STATE_READY)
		return;
	if (retries > 10)
		state = STATE_SEND_RESET;
	switch (state) {
	case STATE_SEND_RESET:
		reset_controller();
		next_timeout = 5;
		state = STATE_UNKNOWN;
		retries = 0;
		break;

	case STATE_UNKNOWN:
		response_received = retries = 0;
		state = STATE_SREQUEST;
		/* Fall through */

	case STATE_SREQUEST:
		if (response_received == 0) {
			next_timeout = 3;
			syslog(LOG_DEBUG, "Requesting status.\n");
			request_local_status(RADIO_STATUS_STATIC);
			retries++;
			break;
		}
		syslog(LOG_DEBUG, "Status response received.\n");
		state = STATE_DREQUEST;
		response_received = retries = 0;
		/* Fall through */

	case STATE_DREQUEST:
		next_timeout = 5;
		if (response_received == 0) {
			request_local_status(RADIO_STATUS_DYNAMIC);
			retries++;
			break;
		}
		syslog(LOG_DEBUG, "Dynamic response received.\n");
		syslog(LOG_DEBUG, "Last received node: %d\n", response_node);
		retries = 0;
		if (response_node == 1) {
			/*
			 * Don't need to activate this node.
			 */
			state = STATE_SET_TIME;
			failure_status = -1;
			set_time();
			break;
		}
		state = STATE_ACTIVATE;
		failure_status = -1;
		/* Fall through */

	case STATE_ACTIVATE:
		next_timeout = 5;
		syslog(LOG_DEBUG, "Activate (fs%d)\n", failure_status);
		if (failure_status != 0) {
			local_activate();
			retries++;
			break;
		}
		syslog(LOG_DEBUG, "Activation successful.\n");
		failure_status = -1;
		state = STATE_SET_TIME;
		retries = 0;
		/* Fall through */

	case STATE_SET_TIME:
		syslog(LOG_DEBUG, "Set time. (fs%d)\n", failure_status);
		next_timeout = 5;
		if (failure_status != 0) {
			set_time();
			retries++;
			break;
		}
		syslog(LOG_DEBUG, "Time set.\n");
		failure_status = -1;
		state = STATE_SET_DATE;
		retries = 0;
		/* Fall through */

	case STATE_SET_DATE:
		syslog(LOG_DEBUG, "Set date. (fs%d)\n", failure_status);
		next_timeout = 5;
		if (failure_status != 0) {
			set_date();
			retries++;
			break;
		}
		syslog(LOG_DEBUG, "Date set.\n");
		failure_status = -1;
		state = STATE_ACTIVATE_CH0;
		retries = 0;
		/* Fall through */

	case STATE_ACTIVATE_CH0:
	case STATE_ACTIVATE_CH1:
	case STATE_ACTIVATE_CH2:
	case STATE_ACTIVATE_CH3:
		i = state - STATE_ACTIVATE_CH0;
		chstate = (i == 3) ? LIBRADIO_CHSTATE_READ : LIBRADIO_CHSTATE_EMPTY;
		syslog(LOG_DEBUG, "Activate channel %d (state %d, fs%d)\n", i, chstate, failure_status);
		next_timeout = 5;
		if (failure_status != 0) {
			set_channel(i, chstate);
			retries++;
			break;
		}
		syslog(LOG_DEBUG, "Channel %d activated.\n", i);
		retries = 0;
		if (state == STATE_ACTIVATE_CH3) {
			state = STATE_READY;
			syslog(LOG_INFO, "Communications channels are open and working.");
			dynamic_status_timer();
			break;
		}
		failure_status = -1;
		state++;
		chstate = (i == 3) ? LIBRADIO_CHSTATE_READ : LIBRADIO_CHSTATE_EMPTY;
		set_channel(state - STATE_ACTIVATE_CH0, chstate);
		break;

	default:
		syslog(LOG_DEBUG, "Unknown state %d\n", state);
		state = STATE_UNKNOWN;
		break;
	}
	syslog(LOG_DEBUG, "Call me again in %d seconds\n", next_timeout);
	timer_insert(state_machine, next_timeout);
}

/*
 * Is the system in an operational state?
 */
int
state_ready()
{
	return(state == STATE_READY);
}

/*
 * Request dynamic status from the local controller. This runs every five minutes.
 */
void
dynamic_status_timer()
{
	syslog(LOG_DEBUG, "Requesting dynamic status (timer).\n");
	request_local_status(RADIO_STATUS_DYNAMIC);
	timer_insert(dynamic_status_timer, 300);
}
