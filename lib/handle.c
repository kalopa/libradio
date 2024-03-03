/*
 * Copyright (c) 2019-24, Kalopa Robotics Limited.  All rights reserved.
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
 * Handle a received packet.
 */
#include <stdio.h>
#include <avr/io.h>

#include <libavr.h>

#include "libradio.h"
#include "internal.h"

struct channel	rxchan;
struct channel	txchan;

/*
 * Receive a packet from the radio, and call libradio_command() with
 * the received packet if the packet was intended for us.
 */
void
libradio_handle_packet()
{
	struct channel *chp = &rxchan;

	/*
	 * Look for a received packet.
	 */
	if (libradio_recv(chp, radio.my_channel)) {
		if (chp->packet.node == 0 || chp->packet.node == radio.my_node_id)
			libradio_command(&chp->packet);
	}
}

/*
 * Send a response packet to the remote channel/address. Used whenever
 * we receive a request for information such as a STATUS request or an
 * EEPROM read request.
 */
void
libradio_send_response(uchar_t cmd, uchar_t chan, uchar_t addr, uchar_t len, uchar_t buffer[])
{
	int i;
	struct channel *chp = &txchan;

	chp->state = LIBRADIO_CHSTATE_TRANSMIT;
	chp->priority = 10;
	chp->packet.node = addr;
	chp->packet.len = len;
	chp->packet.cmd = cmd;
	if (len > MAX_PAYLOAD_SIZE)
		len = MAX_PAYLOAD_SIZE;
	for (i = 0; i < len; i++)
		chp->packet.data[i] = buffer[i];
	if (libradio_send(chp, chan) != 0) {
		chp->state = LIBRADIO_CHSTATE_EMPTY;
		chp->priority = 0;
	}
	/*
	 * Busy-wait for the transmission to end. Ideally we'd use the
	 * state machine and not go listening for new RX packets until
	 * this was done (or one clock tick).
	 */
	while (libradio_request_device_status() == SI4463_STATE_TX)
		;
	libradio_recv_start();
}
