/*
 * Copyright (c) 2020-24, Kalopa Robotics Limited.  All rights reserved.
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
 * This is the operational code (and state machine) for the main
 * controller.
 */
#include <stdio.h>
#include <avr/io.h>

#include <libavr.h>

#include "libradio.h"
#include "internal.h"
#include "control.h"

#define SET_TIME_MODULO		500

struct channel	channels[MAX_RADIO_CHANNELS];
struct channel	*resp_chp;
struct packet	*pp;

/*
 * Initialize operations. We send time stamps on each channel in and around the
 * even second, and twice per ten minute interval.
 */
void
tx_init()
{
	int i;
	struct channel *chp;

	for (i = 0, chp = channels; i < MAX_RADIO_CHANNELS; i++) {
		chp->state = LIBRADIO_CHSTATE_DISABLED;
		chp->priority = 0;
	}
	resp_chp = NULL;
}

/*
 * Check to see if we need to send a packet on an active channel. Also, send
 * a time sync on a periodic basis.
 */
void
tx_check_queues()
{
	int channo, modulo;
	struct channel *chp;
	static int last_modulo = 0;

	if (radio.state < LIBRADIO_STATE_LISTEN)
		return;
	/*
	 * First off, send a time stamp on each of the active channels regardless
	 * of anything else.
	 */
	modulo = radio.ms_ticks % SET_TIME_MODULO;
	chp = NULL;
	if (modulo < last_modulo) {
		/*
		 * Millisecond clock has wrapped around. Time to TX a SET TIME. This
		 * will generally happen every 5 seconds (SET_TIME_MODULO in ms_ticks).
		 */
		channo = (radio.ms_ticks / SET_TIME_MODULO) % MAX_RADIO_CHANNELS;
		chp = &channels[channo];
		if (chp->state == LIBRADIO_CHSTATE_EMPTY) {
			struct packet *pp;

			printf("C%d>state:%d\n", channo, chp->state);
			/*
			 * Send a "tens of minutes" time packet.
			 */
			chp->state = LIBRADIO_CHSTATE_TRANSMIT;
			chp->priority = 0xff;
			pp = &chp->packet;
			pp->node = 0;
			pp->len = 1;
			pp->cmd = RADIO_CMD_SET_TIME;
			pp->data[0] = radio.tens_of_minutes;
		}
	}
	last_modulo = modulo;
	if (chp == NULL) {
		struct channel *nchp;

		/*
		 * No time of day packet so choose the highest priority for the
		 * transmission. If we can't find a packet to send, we're done.
		 */
		for (channo = 0, nchp = NULL, chp = channels;
								channo < MAX_RADIO_CHANNELS;
								channo++, chp++) {
			if (chp->priority == 0 || chp->state < LIBRADIO_CHSTATE_TRANSMIT)
				continue;
			if (nchp == NULL || chp->priority > nchp->priority)
				nchp = chp;
		}
		chp = nchp;
	}
	if (chp == NULL || chp->state < LIBRADIO_CHSTATE_EMPTY)
		return;
	/*
	 * We have a channel ready for transmission. Send it now...
	 */
	channo = chp - channels;
	printf("TX%d:chst:%d,C%d,len%d\n", channo, chp->state, chp->packet.cmd, chp->packet.len);
	if (libradio_send(chp, channo) != 0) {
		if (chp->state == LIBRADIO_CHSTATE_TXRESPOND) {
			/*
			 * Once transmission has ended, the radio will
			 * immediately go to RX mode. Enable the IRQ
			 * and wait for the response. Save the channel
			 * pointer so the main code knows not to send
			 * a transmission until we've received the reply.
			 */
			libradio_irq_enable(1);
			chp->state = LIBRADIO_CHSTATE_RXRESPONSE1;
			resp_chp = chp;
		} else
			chp->state = LIBRADIO_CHSTATE_EMPTY;
		chp->priority = 0;
	}
	/*
	 * Now increment the priority of the remaining channels to prevent them
	 * getting locked out by busy channels at a higher priority.
	 */
	for (channo = 0, chp = channels; channo < MAX_RADIO_CHANNELS; channo++, chp++) {
		if (chp->state >= LIBRADIO_CHSTATE_TRANSMIT && chp->priority < 0xfc)
			chp->priority += 2;
	}
}
