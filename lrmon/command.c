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
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <syslog.h>

#include "lrmon.h"
#include "libradio.h"

int		failure_status;

/*
 *
 */
void
local_activate()
{
	int data[3];

	data[0] = 0;		/* My channel */
	data[1] = 1;		/* My Node ID */
	data[2] = 0;		/* ??? */
	syslog(LOG_DEBUG, "Local activation request to channel %d, node %d\n", data[0], data[1]);
	send_command(0, 0, RADIO_CMD_ACTIVATE, data, 3);
}

/*
 *
 */
void
client_activate(int data[], int dlen)
{
	syslog(LOG_DEBUG, "Client activation.\n");
	send_command(0, 0, RADIO_CMD_ACTIVATE, data, dlen);
}

/*
 * Request the status of the control system.
 */
void
request_local_status(int type)
{
	char buffer[4];

	syslog(LOG_DEBUG, "Requesting local status %d.\n", type);
	buffer[0] = '>';
	buffer[1] = type + 'S';
	buffer[2] = '\0';
	sio_send(buffer);
}

/*
 *
 */
void
request_client_status(int chan, int node, int type)
{
	int data[3];

	data[0] = 1;		/* Response channel */
	data[1] = 1;		/* Response node */
	data[2] = type;
	syslog(LOG_DEBUG, "Request client status %d, %d, %d\n", chan, node, type);
	send_command(chan, node, RADIO_CMD_STATUS, data, 3);
}

/*
 *
 */
void
set_time()
{
	int ticks, tod, data[3];
	struct timeval tval;

	syslog(LOG_DEBUG, "Set time.\n");
	if (gettimeofday(&tval, NULL) < 0)
		return;
	ticks = tval.tv_sec % (60*60*24);
	syslog(LOG_DEBUG, "ticks: %d.%06ld\n", ticks, tval.tv_usec);
	//2024-10-14T14:33:39.326580+00:00 grover lrmond[2059518]: ticks: 52419.323578
	tod = (ticks / 600);
	ticks = (ticks % 600) * 100;
	syslog(LOG_DEBUG, "tod: %d, ticks %02d\n", tod, ticks);
	ticks += (tval.tv_usec / 10000);
	syslog(LOG_DEBUG, "ticks: %d\n", ticks);
	data[0] = (ticks >> 8) & 0xff;
	data[1] = ticks & 0xff;
	data[2] = tod;
	send_command(0, 1, RADIO_CMD_SET_TIME, data, 3);
}

/*
 *
 */
void
set_date()
{
	int date, data[2];
	time_t now;

	syslog(LOG_DEBUG, "Set date.\n");
	time(&now);
	date = now / (60*60*24);
	syslog(LOG_DEBUG, "Date: %d\n", date);
	data[0] = (date >> 8) & 0xff;
	data[1] = date & 0xff;
	send_command(0, 1, RADIO_CMD_SET_DATE, data, 2);
}

/*
 *
 */
void
set_channel(int chan, int state)
{
	int data[2];

	syslog(LOG_DEBUG, "Set channel %d to state %d\n", chan, state);
	data[0] = chan;
	data[1] = state;
	send_command(0, 1, RADIO_CMD_USER0, data, 2);
}

/*
 *
 */
void
send_command(int chan, int node, int cmd, int data[], int dlen)
{
	int i;
	char *cp, obuffer[1024];

	syslog(LOG_DEBUG, "Sending command %d to node %d on channel %d\n", cmd, node, chan);
	sprintf(obuffer, ">%c%d:%d", chan + 'A', node, cmd);
	for (i = 0, cp = obuffer + strlen(obuffer); i < dlen; i++) {
		if (i == 0)
			*cp++ = ':';
		else
			*cp++ = ',';
		sprintf(cp, "%d", data[i]);
		cp += strlen(cp);
	}
	*cp++ = '.';
	*cp++ = '\0';
	sio_send(obuffer);
	syslog(LOG_DEBUG, "Back from send...\n");
}

/*
 *
 */
void
reset_controller()
{
	syslog(LOG_DEBUG, "Hard-reset the controller.\n");
	sio_send(">R");
}
