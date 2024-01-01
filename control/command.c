/*
 * Copyright (c) 2020-23, Kalopa Robotics Limited.  All rights reserved.
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
	case RADIO_CMD_FIRMWARE:
	case RADIO_CMD_DEACTIVATE:
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
		if (pp->len != 3)
			break;
		printf("M-Actvt! ch:%d,node:%d\n", pp->data[0], pp->data[1]);
		radio.my_channel = pp->data[0];
		radio.my_node_id = pp->data[1];
		if (libradio_power_up() == 0) {
			printf("Going to ACTIVE state.\n");
			libradio_set_state(LIBRADIO_STATE_ACTIVE);
		} else {
			printf("Radio power-up failed.\n");
			return(6);
		}
		break;

	case RADIO_CMD_SET_TIME:
		/*
		 * Set the "tens of minutes" value thus enabling the real-time clock.
		 * We handle this differently to the normal routine because the
		 * millisecond timer is also sent (via the serial port). Normally this
		 * is implied in the packet.
		 */
		if (pp->len != 3)
			break;
		radio.ms_ticks = (pp->data[0] << 8 | pp->data[1]);
		radio.tens_of_minutes = pp->data[2];
		printf(">> Ctlr Time:%u/%u\n", radio.ms_ticks, radio.tens_of_minutes);
		break;

	case RADIO_CMD_STATUS:
		/*
		 * Report status back over the serial line.
		 */
		if (pp->len != 3)
			break;
		local_status(pp->data[2]);
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
		addr = (pp->data[3] << 8 | pp->data[4]);
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
		printf("CHn:");
		for (i = 0; i < MAX_RADIO_CHANNELS; i++)
			printf("%d.", channels[i].state);
		printf("..%u\n", radio.heart_beat);
		break;

	default:
		return(7);
	}
	return(0);
}

/*
 *
 */
void
local_status(uchar_t stype)
{
	int i, bv, len = 0;
	uchar_t status[10];

	switch (stype) {
	case RADIO_STATUS_DYNAMIC:
		bv = analog_read(3);
		status[0] = radio.state;
		status[1] = radio.tens_of_minutes;
		status[2] = (bv >> 8) & 0xff;
		status[3] = (bv & 0xff);
		status[4] = (radio.npacket_rx >> 8) & 0xff;
		status[5] = (radio.npacket_rx & 0xff);
		status[6] = (radio.npacket_tx >> 8) & 0xff;
		status[7] = (radio.npacket_tx & 0xff);
		len = 8;
		break;

	case RADIO_STATUS_STATIC:
		status[0] = CONTROL_C1;
		status[1] = CONTROL_C2;
		status[2] = CONTROL_N1;
		status[3] = CONTROL_N2;
		status[4] = FW_VERSION_H;
		status[5] = FW_VERSION_L;
		len = 6;
		break;
	}
	printf("<%c%d:%u:%d:%d", radio.my_channel + 'A', radio.my_node_id,
							radio.ms_ticks, RADIO_STATUS_RESPONSE, stype);
	for (i = 0; i < len; i++)
		printf(",%u", status[i]);
	putchar('\n');
}
