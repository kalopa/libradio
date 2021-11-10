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
 * This file holds the main command processing function. This is shared
 * across all the clients, and handles the basic (built-in) command set
 * such as activating and de-activating. It also manages keeping track
 * of the date and time, as well as some utility functions for reading
 * and writing EEPROM data. Everything else is handled by the actual
 * application in the operate() function.
 */
#include <stdio.h>
#include <avr/io.h>

#include <libavr.h>

#include "libradio.h"
#include "internal.h"

/*
 * Handle a received command from the radio. We deal with the lower set of
 * commands (0->7 currently) internally, and pass any other commands on up
 * to the main system via the operate() function.
 */
void
libradio_command(struct packet *pp)
{
	int i, len, addr, rchan, rnode;

	printf("Received a command: %d (len:%d)\n", pp->cmd, pp->len);
	switch (pp->cmd) {
	case RADIO_CMD_NOOP:
		break;

	case RADIO_CMD_FIRMWARE:
		/*
		 * Do a firmware update. Ignore - for now.
		 */
		break;

	case RADIO_CMD_STATUS:
		/*
		 * Return a status packet on the specified channel.
		 */
		if (pp->len != 3)
			break;
		rchan = pp->data[0];
		rnode = pp->data[1];
		i = pp->data[2];
		printf(">> Send status type %d to %d:%d\n", i, rchan, node);
		//fetch_status(i, &statusbuffer);
		//send...
		break;

	case RADIO_CMD_ACTIVATE:
		/*
		 * An activation request! See if it's for us, and if so, activate.
		 */
		if (pp->len != 6)
			break;
		printf(">> Activate! %d/%d/%d/%d\n", radio.cat1, radio.cat2, radio.num1, radio.num2);
		if (pp->data[2] == radio.cat1 &&
					pp->data[3] == radio.cat2 &&
					pp->data[4] == radio.num1 &&
					pp->data[5] == radio.num2) {
			radio.my_channel = pp->data[0];
			radio.my_node_id = pp->data[1];
			printf("ACTIVATED! [C%dN%d]\n", radio.my_channel, radio.my_node_id);
		}
		libradio_set_state(LIBRADIO_STATE_ACTIVE);
		break;

	case RADIO_CMD_DEACTIVATE:
		/*
		 * Deactivate this device.
		 */
		if (pp->len != 0)
			break;
		printf(">> Deactivate...\n");
		radio.my_channel = radio.my_node_id = 0;
		libradio_set_state(LIBRADIO_STATE_WARM);
		break;

	case RADIO_CMD_SET_TIME:
		/*
		 * Set the "tens of minutes" value thus enabling the real-time clock.
		 */
		if (pp->len != 1)
			break;
		radio.tens_of_minutes = pp->data[0];
		printf(">> Set Tens of Minutes:%u\n", radio.tens_of_minutes);
		break;

	case RADIO_CMD_SET_DATE:
		/*
		 * Set the date. We don't use this information, it is
		 * application-specific.
		 */
		if (pp->len != 2)
			break;
		radio.date = (pp->data[1] << 8 | pp->data[0]);
		printf(">> Set Date: %u\n", radio.date);
		break;

	case RADIO_CMD_READ_EEPROM:
		/*
		 * Read up to 16 bytes of EEPROM data. The result is saved in a
		 * packet buffer and transmitted on the specified channel.
		 */
		if (pp->len != 5)
			break;
		printf(">> Read EEPROM...\n");
		rchan = pp->data[0];
		rnode = pp->data[1];
		len = pp->data[2];
		addr = (pp->data[4] << 8 | pp->data[3]);
		printf("RChan/Node %d:%d, len:%d, addr:%d\n", rchan, node, len, addr);
#if 0
		for (i = 0; i < len; i++, addr++)
			statusbuffer[i] = eeprom_read_byte((const unsigned char *)addr);
		//send...
#endif
		break;

	case RADIO_CMD_WRITE_EEPROM:
		/*
		 * Write up to 16 bytes of EEPROM data.
		 */
		if (pp->len < 4 || pp->len > 19)
			break;
		printf(">> Write EEPROM (%d bytes)...\n", pp->len - 3);
		len = pp->data[0];
		addr = (pp->data[2] << 8 | pp->data[1]);
		printf("Len: %d, addr: %d\n", len, addr);
		for (i = 0; i < len; i++, addr++)
			eeprom_write_byte((unsigned char *)addr, pp->data[i + 3]);
		break;

	default:
		/*
		 * For all other command types, just call the client app.
		 */
		operate(pp);
		break;
	}
}
