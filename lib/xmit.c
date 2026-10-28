/*
 * Copyright (c) 2020-25, Kalopa Robotics Limited.  All rights reserved.
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
 * Packet transmission. This is deliberately split into two steps so an
 * application can prepare its data well ahead of its actual slot:
 *
 * 1. libradio_transmit() - called by the app (usually from a slot_func()
 *    callback fired LIBRADIO_TX_LEAD_TICKS before the slot boundary) -
 *    stages the packet into the Si4463's TX FIFO right away and sets
 *    radio.tx_pending.
 * 2. libradio_fire_tx() - called by poll.c exactly at the slot boundary -
 *    issues the short SI4463_START_TX command. This is the only part
 *    that's actually time-critical.
 *
 * No software checksum: the Si4463's own hardware CRC (see
 * RF_PKT_CRC_CONFIG_7 in radio_config.h) validates the packet for us.
 */
#include <stdio.h>
#include <avr/io.h>
#include <string.h>

#include <libavr.h>

#include "libradio.h"
#include "internal.h"

/*
 * Copy a packet's raw bytes into the Si4463's TX FIFO.
 */
uchar_t
libradio_txpacket(struct packet *pp)
{
	int i;
	uchar_t *cp;

	_setss(1);
	spi_byte(SI4463_WRITE_TX_FIFO);
	for (i = 0, cp = (uchar_t *)pp; i < MAX_PACKET_SIZE; i++)
		spi_byte(*cp++);
	_setss(0);
	return(1);
}

/*
 * Stage a packet for transmission. Loads it into the TX FIFO immediately;
 * the actual transmission is triggered later, at the slot boundary, by
 * libradio_fire_tx().
 */
int
libradio_transmit(struct packet *pp)
{
	if (libradio_txpacket(pp) == 0)
		return(0);
	radio.tx_pending = 1;
	return(1);
}

/*
 * Fire a previously-staged transmission. Called by poll.c right at the
 * slot boundary - this is the time-critical ~60us SPI command the design
 * notes call out. Does nothing if nothing was staged.
 */
void
libradio_fire_tx()
{
	if (!radio.tx_pending)
		return;
	pkt_data[0] = SI4463_START_TX;
	pkt_data[1] = RF_CHANNEL;
	/*
	 * Always drop back into RX once the TX completes - every slot but
	 * our own is spent listening, so there's no reason to go idle.
	 */
	pkt_data[2] = (SI4463_STATE_RX << 4);
	pkt_data[3] = 0;
	pkt_data[4] = SI4463_PACKET_LEN;
	TESTPT(1);
	if (pkt_send(5, 0) != SPI_SEND_OK) {
		pkt_error(1);
	} else {
		radio.npacket_tx++;
		/*
		 * A successful transmission means we have a working slot -
		 * that's the last rung of the client up-path state ladder.
		 * (Doesn't apply to the team lead - it's always TEAMLEAD.)
		 */
		if (radio.state == LIBRADIO_STATE_SYNCED)
			libradio_set_state(LIBRADIO_STATE_ONLINE);
	}
	TESTPT(0);
	radio.curr_state = SI4463_STATE_RX;
	radio.tx_pending = 0;
}
