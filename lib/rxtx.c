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
 * Start up the radio in RX mode. If the radio isn't already active,
 * power it up. Set the radio to receive on our chosen (radio.my_channel)
 * channel, and enable RX interrupts.
 */
uchar_t
libradio_recv_start()
{
	if (!radio.radio_active && libradio_power_up() < 0)
		return(0);
	radio.catch_irq = 1;
	libradio_set_rx(radio.my_channel);
	return(1);
}

/*
 * Receive packet data from the radio receiver. Do this by querying the
 * FIFO status, and if there's at least one packet in the FIFO, pull
 * it out. If the checksum is bad or the retrieval fails, then flush
 * the RX FIFO. If we're using RX interrupts, re-enable them. Finally,
 * if we're no longer in RX mode, then re-enable it.
 */
uchar_t
libradio_recv(struct channel *chp, uchar_t channo)
{
	uchar_t ret = 0;

	/*
	 * First up, check the RX fifo to see if we have a packet.
	 */
	if (libradio_check_rx()) {
		/*
		 * There's at least a packet. Go get it!
		 */
		if ((ret = libradio_rxpacket(chp)) == 0) {
			/*
			 * Got a defective packet - clear the RX FIFO
			 * in case there's more. This is a particularly
			 * awkward thing to do because the FIFO may be
			 * in the throes of receiving the next packet. In
			 * which case, we'll end up with a partial packet
			 * in the FIFO and we'll never get it out. Ideally
			 * here some sort of "I haven't received data
			 * in X seconds, so flush any crud out of the
			 * FIFO" might not be a bad way to go. Or if
			 * the FIFO has had less than a packet-load for
			 * N milliseconds, flush it.
			 */
			libradio_get_fifo_info(02);
		} else {
			radio.npacket_rx++;
			radio.saw_rx = 1;
		}
		/*
		 * If we caught an IRQ notification, re-enable interrupts now that
		 * we've pulled the packet.
		 */
		if (radio.catch_irq)
			libradio_irq_enable(1);
	}
	if (libradio_request_device_status() != SI4463_STATE_RX ||
					radio.curr_channel != channo)
		libradio_set_rx(channo);
	return(ret);
}

/*
 * Put the radio into RX mode.
 */
void
libradio_set_rx(uchar_t channo)
{
	int i;

	pkt_data[0] = SI4463_START_RX;
	pkt_data[1] = channo;
	pkt_data[2] = 0;		/* CONDITION: Start immediately */
	pkt_data[3] = 0;		/* RXLen(hi) */
	pkt_data[4] = SI4463_PACKET_LEN;
	pkt_data[5] = SI4463_STATE_NOCHANGE;
	pkt_data[6] = SI4463_STATE_READY;
	pkt_data[7] = SI4463_STATE_READY;
	if ((i = pkt_send(8, 0)) != SPI_SEND_OK)
		pkt_error(i);
	radio.curr_channel = channo;
}

/*
 * Transmit data via the radio transmitter. Note that if it is still powering
 * up then we'll need to wait a bit. Likewise, if the radio is asleep, we
 * will need to bring it back online. Only send the packet if we are in a
 * READY state and the FIFO has space.
 */
uchar_t
libradio_send(struct channel *chp, uchar_t channo)
{
	int i;

	/*
	 * Power up the radio, if necessary.
	 */
	if (!radio.radio_active && libradio_power_up() < 0)
		return(0);
	/*
	 * This seems odd, checking that we're in a READY state but it's
	 * to avoid the situation where we're currently transmitting. We
	 * could be in an RX state if we're waiting for a response packet
	 * from the client.
	 */
	libradio_request_device_status();
	if (radio.curr_state != SI4463_STATE_READY && radio.curr_state != SI4463_STATE_RX)
		return(0);
	/*
	 * Check we have sufficient space in the transmit FIFO for
	 * the packet.
	 */
	if (libradio_check_tx() == 0)
		return(0);
	/*
	 * Finally! We're clear for launch! Transmit the packet contents
	 * to the TX FIFO and spin up a TRANSMIT request. Note that if
	 * the packet type indicates a response is required, then switch
	 * to RX mode afterwards. Otherwise, drop back to READY.
	 */
	if (libradio_txpacket(chp) == 0)
		return(0);
	pkt_data[0] = SI4463_START_TX;
	pkt_data[1] = channo;
	if (chp->state > LIBRADIO_CHSTATE_TRANSMIT)
		i = SI4463_STATE_RX;
	else
		i = SI4463_STATE_READY;
	pkt_data[2] = (i << 4);
	pkt_data[3] = 0;
	pkt_data[4] = SI4463_PACKET_LEN;
	i = pkt_send(5, 0);
	if (i != SPI_SEND_OK)
		return(pkt_error(i));
	radio.npacket_tx++;
	return(1);
}

/*
 * Check if we have received at least one packet.
 */
uchar_t
libradio_check_rx()
{
	return(libradio_get_fifo_info(0) >= SI4463_PACKET_LEN);
}

/*
 * Check if we have received at least one packet.
 */
uchar_t
libradio_check_tx()
{
	libradio_get_fifo_info(0);
	return(radio.tx_fifo >= SI4463_PACKET_LEN);
}
