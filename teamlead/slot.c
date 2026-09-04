/*
 * Copyright (c) 2020-25, Kalopa Robotics Limited.  All rights reserved.
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
 * of its active channels at regular intervals.
 */
#include <stdio.h>
#include <avr/io.h>
#include <string.h>

#include <libavr.h>

#include "libradio.h"
#include "teamlead.h"

struct packet	my_packets[9];

/*
 * Time for a slot transmission.
 */
void
slot_event(int slotid)
{
	struct packet *pp;

	switch (slotid) {
	case LIBRADIO_SLOT_BEACON:
		/*
		 * Time to send a beacon packet.
		 */
		pp = &my_packets[0];
		pp->rxchan = LIBRADIO_CHANNEL_BROADCAST;
		pp->command = RADIO_CMD_BEACON;
		memcpy(pp->data, libradio_get_assignments(), MAX_PAYLOAD_SIZE);
		libradio_transmit(pp);
		break;

	case LIBRADIO_SLOT_TIMESTAMP:
		/*
		 * Time to send a timestamp packet.
		 */
		pp = &my_packets[0];
		pp->rxchan = LIBRADIO_CHANNEL_BROADCAST;
		pp->command = RADIO_CMD_TIMESTAMP;
		pp->data[0] = 0;
		pp->data[1] = 0;
		pp->data[2] = 0;
		pp->data[3] = 0;
		libradio_transmit(pp);
		break;
	}
}
