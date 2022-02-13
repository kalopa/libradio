/*
 * Copyright (c) 2019-21, Kalopa Robotics Limited.  All rights reserved.
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

/*
 *
 */
void
libradio_handle_packet(struct channel *chp)
{
	int i;
	struct packet *pp;

	/*
	 * Look for a received packet.
	 */
	printf("HP:%d\n", radio.my_channel);
	if (libradio_recv(chp, radio.my_channel)) {
		printf("\nPacket RX! (tick:%u),len:%d\n", radio.ms_ticks, chp->offset);
		for (i = 0; i < chp->offset;) {
			pp = (struct packet *)&chp->payload[i];
			printf("N%d,L%d,C%d\n", pp->node, pp->len, pp->cmd);
			if (pp->len == 0)
				break;
			if (pp->node == 0 || pp->node == radio.my_node_id)
				libradio_command(pp);
			i += pp->len;
		}
	}
}

/*
 *
 */
void
libradio_send_response(uchar_t chan, uchar_t addr, uchar_t len, uchar_t buffer[])
{
	printf("TX Response on C%dN%d, len: %d\n", chan, addr, len);
}
