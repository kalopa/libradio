/*
 * Copyright (c) 2020-24, Kalopa Robotics Limited.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ABSTRACT
 * This is the main radio controller. On any given radio network,
 * you need at least one radio controller. It is in charge of managing
 * all of the client devices. It communicates with a more intelligent
 * system via the serial port, and schedules radio transmissions on all
 * of its active channels at regular intervals. It uses the main library
 * for a lot of the low-level radio functions but the operational state
 * machine is quite different. For example, if it is in a COLD or WARM
 * sleep, it is the serial port which will re-activate the software,
 * not the radio interface.
 */
#include <stdio.h>
#include <avr/io.h>
#include <string.h>

#include <libavr.h>

#include "libradio.h"
#include "internal.h"
#include "control.h"

/*
 * Deal with a status response from a client. We asked for a STATUS update or
 * EEPROM data and now the client has sent us what we wanted. Forward the
 * packet up via the RS232 line and go back to TX mode.
 */
void
txresponse(struct channel *chp)
{
	int i, len, from_chan = 0, from_node = 0;
	struct packet *pp = &chp->packet;

	from_chan = libradio_get_my_channel();
	from_node = chp->packet.node;
	printf("RCV from %d on ch%d\n", from_node, from_chan);
	if (libradio_recv(chp, from_chan) == 0)
		return;
	printf("<%c%d:%u:%d:", from_chan + 'A', from_node, pp->ticks, pp->cmd);
	if ((len = pp->len) > MAX_PAYLOAD_SIZE)
		len = MAX_PAYLOAD_SIZE;
	for (i = 0; i < len; i++) {
		if (i > 0)
			putchar(',');
		printf("%d", pp->data[i]);
	}
	putchar('\n');
}
