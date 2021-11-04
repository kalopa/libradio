/*
 * Copyright (c) 2020-21, Kalopa Robotics Limited.  All rights
 * reserved.
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
 * All radio communications happens here.
 * 434.00 to 434.80 MHz.
 */
#include <stdio.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

#include <libavr.h>

#include "libradio.h"
#include "radio.h"

/*
 * Receive data from the radio receiver. Note that if it is still powering
 * up then we'll need to wait a bit. Likewise, if the radio is asleep, we
 * will need to bring it back online.
 */
uchar_t
libradio_recv(struct libradio *rp, struct channel *chp, uchar_t channo)
{
	uchar_t ret = 0;

	chp->offset = 0;
	printf("RECV!\n");
	if (!rp->radio_active && libradio_power_up(rp) < 0)
		return(0);
	/*
	 * First up, check the RX fifo to see if we have a packet.
	 */
	libradio_get_int_status(rp);
	libradio_get_fifo_info(rp, 0);
	if (rp->rx_fifo >= SI4463_PACKET_LEN) {
		/*
		 * There's at least a packet. Go get it!
		 */
		spi_rxpacket(rp, chp);
		ret = 1;
	}
	if (libradio_request_device_status(rp) != SI4463_STATE_READY &&
								rp->curr_channel == channo)
		return(ret);
	/*
	 * We're in a READY state. Let's get receiving...
	 */
	spi_data[0] = SI4463_START_RX;
	spi_data[1] = channo;
	spi_data[2] = 0;		/* CONDITION: Start immediately */
	spi_data[3] = 0;		/* RXLen(hi) */
	spi_data[4] = SI4463_PACKET_LEN;
	spi_data[5] = SI4463_STATE_NOCHANGE;
	spi_data[6] = SI4463_STATE_READY;
	spi_data[7] = SI4463_STATE_READY;
	spi_send(8, 0);
	rp->curr_channel = channo;
	return(ret);
}
