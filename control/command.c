/*
 * Copyright (c) 2020-21, Kalopa Robotics Limited.  All rights reserved.
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
 * This is the main radio controller. On any given radio network,
 * you need at least one radio controller. It is in charge of managing
 * all of the client devices. It communicates with a more intelligent
 * system via the serial port, and schedules radio transmissions on all
 * of its active channels at regular intervals. It uses the main library
 * for a lot of the low-level radio functions but the operational state
 * machine is quite different. For example, if it is in a COLD or WARM
 * sleep, it is the serial port which will re-activate the software,
 * not the radiointerface.
 */
#include <stdio.h>
#include <avr/io.h>
#include <avr/eeprom.h>
#include <string.h>

#include <libavr.h>

#include "libradio.h"
#include "internal.h"
#include "control.h"

/*
 * We have two application-specific commands, which allow the upstream
 * overlords to specify what channels we can use.
 */
#define RADIO_CMD_SET_CHANNEL		(RADIO_CMD_ADDITIONAL_BASE+0)

/*
 * Execute a packet command, locally. For the most part, we try to just use
 * the code in libradio_command() but that won't work for things which send
 * data back, or things like activation where we don't care about the actual
 * IDs sent - they came over the serial line, we know it's for us!
 */
uchar_t
mycommand(struct packet *pp)
{
	int i, len, addr, rchan, rnode;


	switch (pp->cmd) {
	case RADIO_CMD_NOOP:
	case RADIO_CMD_DEACTIVATE:
	case RADIO_CMD_SET_TIME:
	case RADIO_CMD_SET_DATE:
	case RADIO_CMD_WRITE_EEPROM:
		/*
		 * Use the library mechanism for these commands.
		 */
		libradio_command(pp);
		break;

	case RADIO_CMD_ACTIVATE:
		/*
		 * Activate this device. Don't even check the IDs, just do it.
		 */
		if (pp->len != 6)
			break;
		printf("Master Activate!\n");
		radio.my_channel = pp->data[0];
		radio.my_node_id = pp->data[1];
		libradio_set_state(LIBRADIO_STATE_ACTIVE);
		break;

	case RADIO_CMD_STATUS:
		/*
		 * Report status back over the serial line.
		 */
		if (pp->len != 3)
			break;
		rchan = pp->data[0];
		rnode = pp->data[1];
		report(rchan, pp->cmd);
		break;

	case RADIO_CMD_READ_EEPROM:
		/*
		 * Read up to 16 bytes of EEPROM data. The result is saved in a
		 * packet buffer and transmitted on the specified channel.
		 */
		if (pp->len != 5)
			break;
		rchan = pp->data[0];
		rnode = pp->data[1];
		len = pp->data[2];
		addr = (pp->data[4] << 8 | pp->data[3]);
		printf("RChan/Node %d:%d, len:%d, addr:%d\n", rchan, rnode, len, addr);
		printf("<%c:%d:", rchan + 'A', rnode);
		for (i = 0; i < len; i++, addr++) {
			if (i != 0)
				putchar(',');
			printf("%u", eeprom_read_byte((const unsigned char *)addr));
		}
		putchar('\n');
		break;

	case RADIO_CMD_SET_CHANNEL:
		/*
		 * The SET CHANNEL command enables, disables, or sets a read
		 * channel. It has two arguments - the channel number, and
		 * the state.
		 */
		if (pp->len != 2)
			break;
		if (pp->data[0] < 0 || pp->data[0] > MAX_RADIO_CHANNELS)
			break;
		set_channel(pp->data[0], pp->data[1]);
		break;

	default:
		return(0);
	}
	return(1);
}

/*
 *
 */
void
report(uchar_t rchan, uchar_t rtype)
{
	printf("<%c%d:%d:", rchan + 'A', radio.my_node_id, rtype);
	switch (rtype) {
	case RADIO_CMD_STATUS:
		printf("%d,%d,%d,%u,%u\n", FW_VERSION_H, FW_VERSION_L,
						radio.state, radio.npacket_rx, radio.npacket_tx);
		break;
	}
}
