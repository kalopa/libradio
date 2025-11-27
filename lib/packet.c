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
 * This is where the low-level serial peripheral interface (SPI) activity
 * takes place. Functions to read and write a byte over the SPI port are
 * contained in here, and in the low-level _setss() function. Eventually
 * this stuff will operate via interrupts, but for now it is polled.
 */
#include <stdio.h>
#include <avr/io.h>
#include <libavr.h>

#include "libradio.h"
#include "internal.h"

#define MAX_COUNT		1000

uchar_t		pkt_state;
uchar_t		pkt_index;
uchar_t		pkt_send_len;
uchar_t		pkt_recv_len;
uchar_t		pkt_data[MAX_SPI_BLOCK];

/*
 * Initialize the SPI circuit in the Atmel chip. We are running at a speed
 * of 1MHz with interrupts enabled.
 */
void
pkt_init()
{
	pkt_index = pkt_send_len = pkt_recv_len = 0;
	spi_init();
}

/*
 * Send a sequence (send_len) of bytes to the the radio (in the pkt_data[]
 * buffer) and retrieve a sequence (recv_len) of bytes in response. This
 * can be used to write data (for example to the FIFO), to exchange data
 * (such as sending a command), and to read data (for example, reading
 * from the RX FIFO). For every byte we send, we get one back.
 */
uchar_t
pkt_send(uchar_t send_len, uchar_t recv_len)
{
	int i, j, k;

	/*
	 * Do this piece with the SLAVE_SELECT (SS) bit enabled.
	 */
	_setss(1);
	for (i = 0; i < send_len; i++) {
		if (spi_byte(pkt_data[i]) == -1) {
			_setss(0);
			return(SPI_SEND_WRITE_FAIL);
		}
	}
	_setss(0);
	for (i = 0; i < MAX_COUNT; i++) {
		/*
		 * Start by sending a READ_CMD_BUFF command.
		 */
		_setss(1);
		if (spi_byte(0x44) < 0) {
			_setss(0);
			return(SPI_SEND_STATUS_FAIL);
		}
		if ((k = spi_byte(0xff)) < 0) {
			_setss(0);
			return(SPI_SEND_ACK_FAIL);
		}
		if (k == 0xff) {
			for (j = 0; j < recv_len; j++) {
				if ((k = spi_byte(0xff)) < 0) {
					_setss(0);
					return(SPI_SEND_READ_FAIL);
				}
				pkt_data[j] = k;
			}
			_setss(0);
			break;
		}
		/*
		 * No CTS - try again.
		 */
		_setss(0);
	}
	return(i == MAX_COUNT ? SPI_SEND_TIMEOUT : SPI_SEND_OK);
}

/*
 * Copy a channel packet from the RX FIFO. As the packet is retrieved,
 * validate the checksum, Note that we will overwrite the system clock
 * ticks (ms_ticks) during this function, so beware...
 */
uchar_t
libradio_rxpacket(struct channel *chp)
{
	int i, len;
	uchar_t *cp, csum;

	/*
	 * Pull the entire packet from the RX FIFO
	 */
	if ((len = radio.rx_fifo) > sizeof(struct packet))
		len = sizeof(struct packet);
	_setss(1);
	spi_byte(SI4463_READ_RX_FIFO);
	for (i = 0, cp = (uchar_t *)&chp->packet; i < len; i++)
		*cp++ = spi_byte(0x00);
	_setss(0);
	/*
	 * Now, check the checksum and save the system clock ticks.
	 */
	if (chp->packet.len > MAX_PAYLOAD_SIZE)
		chp->packet.len = MAX_PAYLOAD_SIZE;
	len = PACKET_HEADER_LEN + chp->packet.len;
	for (i = csum = 0, cp = (uchar_t *)&chp->packet; i < len; i++)
		csum ^= *cp++;
	/*
	 * If the checksum is good, save the network time in clock ticks. If
	 * it's bad, set the command to be a No-Op and store the bad CS for
	 * posterity.
	 */
	if (csum != 0x00)
		return(0);
	if (chp->packet.cmd < RADIO_STATUS_RESPONSE) {
		/*
		 * Only accept time values from control
		 */
		cli();
		radio.ms_ticks = chp->packet.ticks;
		sei();
	}
	return(1);
}

/*
 * Copy a channel packet to the TX FIFO. We update the ticks value in
 * the packet at the last possible moment so that it is as accurate
 * as possible. As it's a 16bit quantity, we do this with interrupts
 * disabled. Also, the packet checksum is computed at the same time.
 */
uchar_t
libradio_txpacket(struct channel *chp)
{
	int i, len;
	uchar_t *cp, csum;

	/*
	 * First off, store our current clock timer ticks in the packet
	 * and compute the checksum. The checksum is computed such that
	 * a subsequent XOR of all the bytes in the packet will result
	 * in a value of 0x00.
	 */
	if (chp->packet.len > MAX_PAYLOAD_SIZE)
		return(0);
	len = PACKET_HEADER_LEN + chp->packet.len;
	cli();
	chp->packet.ticks = radio.ms_ticks;
	sei();
	chp->packet.csum = csum = 0x00;
	for (i = 0, cp = (uchar_t *)&chp->packet; i < len; i++)
		csum ^= *cp++;
	chp->packet.csum = csum;
	/*
	 * Now load the packet data into the TX FIFO.
	 */
	_setss(1);
	spi_byte(SI4463_WRITE_TX_FIFO);
	for (i = 0, cp = (uchar_t *)&chp->packet; i < SI4463_PACKET_LEN; i++) {
		if (i < len)
			spi_byte(*cp++);
		else
			spi_byte(0xff);
	}
	_setss(0);
	return(1);
}

/*
 * Print an SPI error and return zero.
 */
uchar_t
pkt_error(uchar_t i)
{
	printf("spi error %d\n", i);
	return(0);
}
