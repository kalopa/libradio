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
 * Packet reception. Mirrors lib/xmit.c: libradio_set_rx() arms the
 * receiver, libradio_check_rx()/libradio_rxpacket() pull a packet out of
 * the RX FIFO once the radio IRQ (see lib/orig-derived radio_irq in
 * locore.S) tells us one has landed. No software checksum - trust the
 * Si4463's hardware CRC.
 */
#include <stdio.h>
#include <avr/io.h>

#include <libavr.h>

#include "libradio.h"
#include "internal.h"

/*
 * Put the radio into RX mode on our fixed RF channel.
 */
void
libradio_set_rx()
{
	pkt_data[0] = SI4463_START_RX;
	pkt_data[1] = RF_CHANNEL;
	pkt_data[2] = 0;		/* CONDITION: Start immediately */
	pkt_data[3] = 0;		/* RXLen(hi) */
	pkt_data[4] = SI4463_PACKET_LEN;
	pkt_data[5] = SI4463_STATE_NOCHANGE;
	pkt_data[6] = SI4463_STATE_RX;
	pkt_data[7] = SI4463_STATE_RX;
	if (pkt_send(8, 0) != SPI_SEND_OK)
		pkt_error(1);
	radio.curr_state = SI4463_STATE_RX;
	libradio_irq_enable(1);
}

/*
 * Do we have a complete packet sitting in the RX FIFO?
 */
uchar_t
libradio_check_rx()
{
	return(libradio_get_fifo_info(0) >= SI4463_PACKET_LEN);
}

/*
 * Pull a packet's raw bytes out of the RX FIFO.
 */
uchar_t
libradio_rxpacket(struct packet *pp)
{
	int i;
	uchar_t *cp;

	_setss(1);
	spi_byte(SI4463_READ_RX_FIFO);
	for (i = 0, cp = (uchar_t *)pp; i < MAX_PACKET_SIZE; i++)
		*cp++ = spi_byte(0x00);
	_setss(0);
	radio.npacket_rx++;
	return(1);
}
