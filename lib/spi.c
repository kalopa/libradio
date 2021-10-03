/*
 * Copyright (c) 2020-21, Kalopa Robotics Limited.  All rights reserved.
 * Unpublished rights reserved under the copyright laws  of the Republic
 * of Ireland.
 *
 * The software contained herein is proprietary to and embodies the
 * confidential technology of Kalopa Robotics Limited.  Possession,
 * use, duplication or dissemination of the software and media is
 * authorized only pursuant to a valid written license from Kalopa
 * Robotics Limited.
 *
 * RESTRICTED RIGHTS LEGEND   Use, duplication, or disclosure by
 * the U.S.  Government is subject to restrictions as set forth in
 * Subparagraph (c)(1)(ii) of DFARS 252.227-7013, or in FAR 52.227-19,
 * as applicable.
 *
 * THIS SOFTWARE IS PROVIDED BY KALOPA ROBOTICS LIMITED "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL KALOPA ROBOTICS LIMITED
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ABSTRACT
 */
#include <stdio.h>
#include <avr/io.h>
#include <libavr.h>

#include "libradio.h"
#include "radio.h"

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
spiinit()
{
	spi_index = spi_send_len = spi_recv_len = 0;
	SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0);
}

/*
 *
 */
uchar_t
spi_byte(uchar_t byte)
{
	SPDR = byte;
	while ((SPSR & (1<<SPIF)) == 0)
		;
	return(SPDR);
}

/*
 *
 */
uchar_t
spi_send(uchar_t send_len, uchar_t recv_len)
{
	int i, j;

	_setss(1);
	for (i = 0; i < send_len; i++)
		spi_byte(spi_data[i]);
	_setss(0);
	for (i = 0; i < MAX_COUNT; i++) {
		/*
		 * Start by sending a READ_CMD_BUFF command.
		 */
		_setss(1);
		spi_byte(0x44);
		if (spi_byte(0xff) == 0xff) {
			for (j = 0; j < recv_len; j++)
				spi_data[j] = spi_byte(0xff);
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
spi_rxpacket(struct libradio *rp, struct channel *chp)
{
	int i;
	uchar_t *cp;
	uint_t myticks;

	_setss(1);
	spi_byte(SI4463_READ_RX_FIFO);
	myticks = spi_byte(0);
	myticks |= (spi_byte(0) << 8);
	chp->offset = rp->rx_fifo - 2;
	printf("OFF:%d\n", chp->offset);
	for (i = 0, cp = chp->payload; i < chp->offset; i++)
		*cp++ = spi_byte(0x00);
	_setss(0);
	cli();
	rp->ms_ticks = myticks;
	sei();
}

/*
 * Copy a channel packet to the TX FIFO. The trick here is that we
 * send the command and we spoof first two bytes, wiwhich are the
 * clock ticks. They're not saved in the packet but instead copied
 * out to the FIFO directly. The advantage to this is the lack of
 * lag.
 */
void
spi_txpacket(struct libradio *rp, struct channel *chp)
{
	int i;
	uchar_t *cp;
	uint_t myticks;

	_setss(1);
	spi_byte(SI4463_WRITE_TX_FIFO);
	cli();
	myticks = rp->ms_ticks;
	sei();
	spi_byte(myticks & 0xff);
	spi_byte((myticks >> 8) & 0xff);
	/*
	 * Send out the packet length, minus the time stamp we've already sent.
	 * Send nulls if we've run out of data.
	 */
	printf("SPI.TX(%d,%u)\n", chp->offset, myticks);
	for (i = 0, cp = chp->payload; i < (SI4463_PACKET_LEN - 2); i++) {
		if (i < chp->offset)
			spi_byte(*cp++);
		else
			spi_byte(0);
	}
	_setss(0);
}
