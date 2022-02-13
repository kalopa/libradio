/*
 * Copyright (c) 2020-21, Kalopa Robotics Limited.  All rights reserved.
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

struct channel	channels[MAX_RADIO_CHANNELS];
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
}

/*
 *
 */
void
tx_check_queues()
{
	int channo, modulo;
	struct channel *chp, *nchp = NULL;
	static int last_modulo = 0;

	if (radio.state < LIBRADIO_STATE_LISTEN)
		return;
	/*
	 * First off, send a time stamp on each of the active channels regardless
	 * of anything else.
	 */
	modulo = radio.ms_ticks % 1000;
	if (last_modulo > modulo) {
		last_modulo = modulo;
		/*
		 * Millisecond clock has wrapped around. Time to Tx a SET TIME.
		 */
		printf("ticks!%u\n", TCCR1B);
		channo = (radio.ms_ticks / 1000) % MAX_RADIO_CHANNELS;
		chp = &channels[channo];
		printf("channo%d,S%d,off:%d\n", channo, chp->state, chp->offset);
		if (chp->state != LIBRADIO_CHSTATE_DISABLED &&
									chp->state != LIBRADIO_CHSTATE_READ &&
									chp->offset < (MAX_FIFO_SIZE - 8)) {
			struct packet *pp;

			printf("Ichp->state:%d (off%d)\n", chp->state, chp->offset);
			/*
			 * Send a "tens of minutes" time packet.
			 */
			if (chp->state == LIBRADIO_CHSTATE_EMPTY)
				chp->offset = 0;
			chp->state = LIBRADIO_CHSTATE_TRANSMIT;
			chp->priority = 0xff;
			pp = (struct packet *)&chp->payload[chp->offset];
			pp->node = 0;
			pp->len = 4;
			pp->cmd = RADIO_CMD_SET_TIME;
			pp->data[0] = radio.tens_of_minutes;
			chp->offset += 4;
		}
	} else {
		/*
		 * No time of day packet so choose the highest priority for the
		 * transmission. If we can't find a packet to send, we're done.
		 */
		last_modulo = modulo;
		for (channo = 0, chp = channels; channo < MAX_RADIO_CHANNELS;
														channo++, chp++) {
			if (chp->priority == 0 || chp->state != LIBRADIO_CHSTATE_TRANSMIT)
				continue;
			if (nchp == NULL || chp->priority > nchp->priority)
				nchp = chp;
		}
		if (nchp == NULL)
			return;
		printf("Echp->state:%d (off%d)\n", chp->state, chp->offset);
		chp = nchp;
	}
	/*
	 * We have a channel ready for transmission. Send it now...
	 */
	printf("TX: ch%d (len:%d)\n", channo, chp->offset);
	if (chp->offset >= MAX_FIFO_SIZE)
		_bootstrap();
	chp->payload[chp->offset++] = 0;
	chp->payload[chp->offset++] = 0;
	if (libradio_send(chp, channo) != 0) {
		printf("TXGood.\n");
		chp->state = LIBRADIO_CHSTATE_EMPTY;
		chp->priority = 0;
	}
	printf("TXBack. %d/%d\n", chp->state, radio.state);
	/*
	 * Now increment the priority of the remaining channels to prevent them
	 * getting locked out by busy channels at a higher priority.
	 */
	for (channo = 0, chp = channels; channo < MAX_RADIO_CHANNELS;
														channo++, chp++) {
		if (chp->state == LIBRADIO_CHSTATE_TRANSMIT && chp->priority < 0xfc)
			chp->priority += 2;
	}
	printf("Done.\n");
}
