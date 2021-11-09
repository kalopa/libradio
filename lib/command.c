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
	switch (pp->cmd) {
	printf("Received a command: %d\n", pp->cmd);
	case RADIO_CMD_FIRMWARE:
		break;

	case RADIO_CMD_STATUS:
		break;

	case RADIO_CMD_ACTIVATE:
		if (pp->len != 8)
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
		printf(">> Deactivated...\n");
		libradio_set_state(LIBRADIO_STATE_WARM);
		break;

	case RADIO_CMD_SET_TIME:
		if (pp->len == 4)
			radio.tens_of_minutes = pp->data[0];
		printf(">> ToM:%u\n", radio.tens_of_minutes);
		break;

	case RADIO_CMD_SET_DATE:
		if (pp->len == 5)
			radio.date = (pp->data[1] << 8 | pp->data[0]);
		printf(">> Date:%u\n", radio.date);
		break;

	case RADIO_CMD_READ_EEPROM:
		printf("READ EEPROM\n");
		break;

	case RADIO_CMD_WRITE_EEPROM:
		printf("WRITE EEPROM\n");
		break;

	default:
		operate(pp);
		break;
	}
}
