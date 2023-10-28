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
 * Receive a packet over the wire, if one is available. It assumes
 * that we are already on the correct channel and that the radio is in
 * RECEIVE mode. If not, it changes the configuration of the radio ready
 * to receive, which will probably happen before the next call.
 */
#include <stdio.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

#include <libavr.h>

#include "libradio.h"
#include "internal.h"

/*
 * Receive data from the radio receiver. Note that if it is still powering
 * up then we'll need to wait a bit. Likewise, if the radio is asleep, we
 * will need to bring it back online.
 */
uchar_t
libradio_recv(struct channel *chp, uchar_t channo)
{
	uchar_t ret = 0;

	chp->offset = 0;
	if (!radio.radio_active && libradio_power_up() < 0)
		return(0);
	/*
	 * First up, check the RX fifo to see if we have a packet.
	 */
	libradio_get_fifo_info(0);
	if (radio.rx_fifo >= SI4463_PACKET_LEN) {
		/*
		 * There's at least a packet. Go get it!
		 */
		PORTC |= 02;
		spi_rxpacket(chp);
		PORTC &= ~02;
		libradio_get_fifo_info(03);
		radio.npacket_rx++;
		radio.saw_rx = 1;
		/*
		 * If we caught an IRQ notification, re-enable interrupts now that
		 * we've pulled the packet.
		 */
		if (radio.catch_irq)
			libradio_irq_enable(1);
		ret = 1;
	}
	libradio_set_rx(channo);
	return(ret);
}

/*
 * Put the radio into RX mode.
 */
void
libradio_set_rx(uchar_t channo)
{
	if (libradio_request_device_status() == SI4463_STATE_RX &&
					radio.curr_channel == channo)
		return;
	printf("Tune Ch%d\n", channo);
	spi_data[0] = SI4463_START_RX;
	spi_data[1] = channo;
	spi_data[2] = 0;		/* CONDITION: Start immediately */
	spi_data[3] = 0;		/* RXLen(hi) */
	spi_data[4] = SI4463_PACKET_LEN;
	spi_data[5] = SI4463_STATE_NOCHANGE;
	spi_data[6] = SI4463_STATE_READY;
	spi_data[7] = SI4463_STATE_READY;
	spi_send(8, 0);
	radio.curr_channel = channo;
}

/*
 * Transmit data via the radio transmitter. Note that if it is still powering
 * up then we'll need to wait a bit. Likewise, if the radio is asleep, we
 * will need to bring it back online. Only send the packet if we are in a
 * READY state and the FIFO is empty.
 */
uchar_t
libradio_send(struct channel *chp, uchar_t channo)
{
	if (!radio.radio_active && libradio_power_up() < 0)
		return(0);
	if (libradio_request_device_status() != SI4463_STATE_READY)
		return(0);
	libradio_get_fifo_info(0);
	if (radio.tx_fifo < SI4463_PACKET_LEN)
		return(0);
	spi_txpacket(chp);
	spi_data[0] = SI4463_START_TX;
	spi_data[1] = channo;
	spi_data[2] = (SI4463_STATE_READY << 4);
	spi_data[3] = 0;
	spi_data[4] = SI4463_PACKET_LEN;
	spi_send(5, 0);
	radio.npacket_tx++;
	return(1);
}
