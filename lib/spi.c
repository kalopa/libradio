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
 * The raw SI4463 command-transaction protocol: send a command (plus
 * arguments) over SPI, then poll (via READ_CMD_BUFF) until the chip
 * signals CTS, then read back its response. This layer knows nothing
 * about our over-the-air packet format - see lib/xmit.c and lib/recv.c
 * for that. spi_init()/spi_byte() live in libavr; _setss() (the
 * board-specific SPI slave-select pin) lives in lib/setss.S.
 */
#include <stdio.h>
#include <avr/io.h>
#include <libavr.h>

#include "libradio.h"
#include "internal.h"

#define MAX_COUNT		1000

uchar_t		pkt_data[MAX_SPI_BLOCK];

/*
 * Initialize the SPI circuit in the Atmel chip.
 */
void
pkt_init()
{
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
 * Print an SPI error and return zero.
 */
uchar_t
pkt_error(uchar_t i)
{
	printf("spi error %d\n", i);
	return(0);
}
