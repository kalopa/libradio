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
 * Called by libradio, once per slot, LIBRADIO_TX_LEAD_TICKS before that
 * slot's boundary. For Milestone 1 the only slot we actually do anything
 * with is the BEACON (0) - everything else (LEADER_CMD, ALOHA, and the
 * 44 client-assignable slots) is reserved in the slot map but not yet
 * handled; the radio just stays in RX for them. The beacon payload is a
 * bare superframe counter for now - the full 14-role compressed
 * assignment table is Milestone 2 work, once dynamic assignment exists.
 */
#include <stdio.h>
#include <avr/io.h>
#include <string.h>

#include <libavr.h>

#include "libradio.h"
#include "teamlead.h"

/*
 * Time for a slot transmission.
 */
void
slot_event(int slotid)
{
	static struct packet beacon;
	static uint_t superframe = 0;

	switch (slotid) {
	case LIBRADIO_SLOT_BEACON:
		/*
		 * Time to send a beacon packet. For M1 this just carries a
		 * superframe counter, so we can confirm on the client side
		 * (and on the scope) that beacons are landing regularly.
		 */
		beacon.rxchan = LIBRADIO_CHANNEL_BROADCAST;
		beacon.command = RADIO_CMD_BEACON;
		beacon.data[0] = (superframe >> 8) & 0xff;
		beacon.data[1] = superframe & 0xff;
		superframe++;
		libradio_transmit(&beacon);
		break;

	default:
		/*
		 * LEADER_CMD, ALOHA, and client slots: nothing to do yet
		 * (Milestone 2+). The radio just stays in RX.
		 */
		break;
	}
}
