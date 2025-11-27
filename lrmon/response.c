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
#include <syslog.h>
#include <string.h>

#include "lrmon.h"
#include "libradio.h"

int				response_received;
int				response_node;
struct sstatus	sstatus;
struct dstatus	dstatus;


void	local_response(char *);

/*
 * Status response received.
 */
void
response(int chan, int node, int ticks, char *argp)
{
	char *json;

	syslog(LOG_DEBUG, "Status response from channel %d, node %d, ticks %d / [%s]\n", chan, node, ticks, argp);
	response_received++;
	response_node = node;
	/*
	 * Transmit the response to any interested parties.
	 */
	if ((json = (char *)malloc(strlen(argp) + 64)) == NULL) {
		syslog(LOG_ERR, "malloc failure in third-party response");
		exit(1);
	}
	sprintf(json, "{\"chan\":%d,\"node\":%d,\"cmd\":%d,\"ticks\":%d,\"data\":[%s]}",
			chan, node, RADIO_STATUS_RESPONSE, ticks, argp);
	rmq_publish(json);
	free(json);
	/*
	 * Handle a local response.
	 */
	if (node < 2)
		local_response(argp);
}

/*
 * A response from our local controller. Log the appropriate data.
 */
void
local_response(char *argp)
{
	int i, n, idata[16];
	char *data[16];

	n = crack(argp, data, 16, ',');
	for (i = 0; i < n; i++)
		idata[i] = atoi(data[i]);
	if (idata[0] == 0 && n == 7) {
		sstatus.c1 = idata[1];
		sstatus.c2 = idata[2];
		sstatus.n1 = idata[3];
		sstatus.n2 = idata[4];
		sstatus.fw_h = idata[5];
		sstatus.fw_l = idata[6];
		syslog(LOG_DEBUG, "Local static response. Device: %d.%d.%d.%d, fw %d.%d\n",
			sstatus.c1, sstatus.c2, sstatus.n1, sstatus.n2,
			sstatus.fw_h, sstatus.fw_l);
	}
	if (idata[0] == 1 && n == 9) {
		dstatus.radio_state = idata[1];
		dstatus.tens_of_minutes = idata[2];
		dstatus.battery_voltage = (idata[3] << 8) | idata[4];
		dstatus.npacket_rx = (idata[5] << 8) | idata[6];
		dstatus.npacket_tx = (idata[7] << 8) | idata[8];
		syslog(LOG_DEBUG, "Local dynamic response: st%d, ToM %d, battery %d, rx %d, tx %d\n",
			dstatus.radio_state, dstatus.tens_of_minutes,
			dstatus.battery_voltage,
			dstatus.npacket_rx, dstatus.npacket_tx);
	}
}
