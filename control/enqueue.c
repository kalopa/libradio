/*
 * Copyright (c) 2020-23, Kalopa Robotics Limited.  All rights reserved.
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
 * Enqueue a packet for transmission. Note that the packet could be for
 * us, in which case we handle it via mycommand().
 */
#include <stdio.h>
#include <avr/io.h>

#include <libavr.h>

#include "libradio.h"
#include "internal.h"
#include "control.h"

/*
 * Add the packet to the queue. Set the state to TRANSMIT if we're ready to
 * send. Deal with the special-case where this is addressed to us and we
 * don't need to transmit it.
 */
void
enqueue(struct channel *chp, struct packet *pp)
{
	if (pp->node == radio.my_node_id ||
				(pp->cmd == RADIO_CMD_ACTIVATE && radio.state < LIBRADIO_STATE_ACTIVE)) {
		/*
		 * This packet is for me. Don't queue it up for transmission.
		 * Instead, execute the command. Also, free up the channel buffer
		 * if this is the only transmission.
		 */
		response(mycommand(pp));
		chp->state = (chp->offset == 0) ? LIBRADIO_CHSTATE_EMPTY : LIBRADIO_CHSTATE_TRANSMIT;
		return;
	}
	/*
	 * Packet is for transmission. Update the channel offset. Note that we
	 * will only accept packets for transmission in an ACTIVE state.
	 */
	if (radio.state != LIBRADIO_STATE_ACTIVE) {
		chp->state = LIBRADIO_CHSTATE_EMPTY;
		response(2);
		return;
	}
	/*
	 * Add the node, length and channel to the length. Then update
	 * the overall offset.
	 */
	pp->len += PACKET_HEADER_SIZE;
	chp->offset += pp->len;
	if (pp->cmd == RADIO_CMD_STATUS)
		chp->state = LIBRADIO_CHSTATE_TXRESPOND;
	else
		chp->state = LIBRADIO_CHSTATE_TRANSMIT;
	/*
	 * Calculate the TX priority. This is channel-dependent with
	 * channel 0 having the highest priority. Multiply this by 8
	 * to allow some "head room" for other channels to get bumped
	 * up.
	 */
	if (chp->priority == 0)
		chp->priority = (MAX_RADIO_CHANNELS - (chp - channels)) << 3;
	else {
		/*
		 * We already have a priority for the channel. Increment it
		 * now that we've added another packet.
		 */
		if (chp->priority < 0xfc)
			chp->priority++;
	}
	response(0);
}

/*
 * Set a channel state to one of {DISABLED, READ, EMPTY}. This is the command
 * used to enable or disable a radio channel.
 */
void
set_channel(uchar_t channo, uchar_t config)
{
	struct channel *chp;

	if (channo < 0 || channo >= MAX_RADIO_CHANNELS)
		return;
	chp = &channels[channo];
	if (config < 3)
		chp->state = config;
	chp->priority = 0;
}
