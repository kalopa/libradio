/*
 * Copyright (c) 2020-23, Kalopa Robotics Limited.  All rights reserved.
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

uchar_t		spi_state;
uchar_t		spi_index;
uchar_t		spi_send_len;
uchar_t		spi_recv_len;
uchar_t		spi_data[MAX_SPI_BLOCK];

/*
 * Initialize the SPI circuit in the Atmel chip. We are running at a speed
 * of 1MHz with interrupts enabled.
 */
void
spi_init()
{
	spi_index = spi_send_len = spi_recv_len = 0;
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0);
}

/*
 * Send and receive a byte over the SPI bus. Only wait about 20ms for the
 * return (64,000 x 5 instructions at 16MHz).
 */
int
spi_byte(uchar_t byte)
{
	unsigned int i;

	SPDR = byte;
	for (i = 0; i < 64000 && (SPSR & (1<<SPIF)) == 0; i++)
		;
	if (i == 64000)
		return(-1);
	return(SPDR);
}

/*
 *
 */
uchar_t
spi_send(uchar_t send_len, uchar_t recv_len)
{
	int i, j, k;

	_setss(1);
	for (i = 0; i < send_len; i++) {
		if (spi_byte(spi_data[i]) == -1) {
			_setss(0);
			return(0);
		}
	}
	_setss(0);
	for (i = 0; i < MAX_COUNT; i++) {
		/*
		 * Start by sending a READ_CMD_BUFF command.
		 */
		_setss(1);
		if (spi_byte(0x44) < 0 || (k = spi_byte(0xff)) < 0) {
			_setss(0);
			return(0);
		}
		if (k == 0xff) {
			for (j = 0; j < recv_len; j++) {
				if ((k = spi_byte(0xff)) < 0) {
					_setss(0);
					return(0);
				}
				spi_data[j] = k;
			}
			_setss(0);
			break;
		}
		/*
		 * No CTS - try again.
		 */
		_setss(0);
	}
	return(i == MAX_COUNT ? 0 : 1);
}

/*
 * Copy a channel packet from the RX FIFO. Note that we will overwrite
 * the system clock ticks during this function, so beware...
 */
void
spi_rxpacket(struct channel *chp)
{
	int i;
	uchar_t *cp, csum;
	uint_t myticks;

	_setss(1);
	spi_byte(SI4463_READ_RX_FIFO);
	myticks = spi_byte(0) << 8;
	myticks |= spi_byte(0);
	csum = spi_byte(0);
	chp->offset = radio.rx_fifo - 3;
	for (i = 0, cp = chp->payload; i < chp->offset; i++, cp++) {
		*cp = spi_byte(0x00);
		csum ^= *cp;
	}
	_setss(0);
	/*
	 * Throw away dud packets.
	 */
	if (csum == 0x00) {
		cli();
		radio.ms_ticks = myticks;
		sei();
	} else
		chp->offset = 0;
}

/*
 * Copy a channel packet to the TX FIFO. The trick here is that we
 * send the command and we spoof first two bytes, wiwhich are the
 * clock ticks. They're not saved in the packet but instead copied
 * out to the FIFO directly. The advantage to this is the lack of
 * lag.
 */
void
spi_txpacket(struct channel *chp)
{
	int i;
	uchar_t *cp, csum;
	uint_t myticks;

	for (csum = i = 0, cp = chp->payload; i < chp->offset; i++)
		csum ^= *cp++;
	csum ^= 0xff;
	_setss(1);
	spi_byte(SI4463_WRITE_TX_FIFO);
	cli();
	myticks = radio.ms_ticks;
	sei();
	spi_byte((myticks >> 8) & 0xff);
	spi_byte(myticks & 0xff);
	spi_byte(csum ^ 0xff);
	/*
	 * Send out the packet length, minus the time stamp we've already sent.
	 * Send nulls if we've run out of data.
	 */
	for (i = 0, cp = chp->payload; i < (SI4463_PACKET_LEN - 3); i++) {
		if (i < chp->offset)
			spi_byte(*cp++);
		else
			spi_byte(0);
	}
	_setss(0);
}
