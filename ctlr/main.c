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
#include <string.h>

#include <libavr.h>

#include "libradio.h"
#include "internal.h"

/*
 * 
 */
int
main()
{
 	/*
	 * Do the device initialization first...
	 */
	_setled(1);
	set_state(SYSTEM_STATE_ERROR);
	clockinit();
	serialinit();
	spiinit();
	(void )fdevopen(sio_putc, sio_getc);
	sei();
	eeprominit();
	/*
	 * Now do the soft init...
	 */
	putchar('\n');
	putchar('\n');
	opinit();
	radioinit();
	report(RADIO_CMD_FIRMWARE);
	set_state(SYSTEM_STATE_STARTUP);
	radio_power_up();
	while (1) {
		main_wait();
		/*
		 * Process some serial port stuff.
		 */
		while (!sio_iqueue_empty())
			process_input();
		if (system_state == SYSTEM_STATE_ACTIVE ||
						system_state == SYSTEM_STATE_SHUTDOWN)
			do_transmit();
	}
}

/*
 * Execute a packet command, locally.
 */
uchar_t
execute(struct packet *pp)
{
	switch (pp->cmd) {
	case RADIO_CMD_FIRMWARE:
	case RADIO_CMD_STATUS:
		report(pp->cmd);
		break;

	case RADIO_CMD_MASTER_ACTIVATE:
		radio.my_node_id = pp->node;
		set_state(SYSTEM_STATE_ACTIVE);
		break;

	case RADIO_CMD_DEACTIVATE:
		/*
		 * Go to SHUTDOWN mode. This involves flushing the transmit queues
		 * and then shutting down the radio. After that, the system goes
		 * to a SLEEP state.
		 */
		set_state(SYSTEM_STATE_SHUTDOWN);
		break;

	case RADIO_CMD_SET_TIME:
		if (pp->len != 3)
			return(0);
		ticks = (pp->data[1] << 8 | pp->data[0]);
		tens_of_minutes = pp->data[2];
		break;

	case RADIO_CMD_SET_DATE:
		if (pp->len != 2)
			return(0);
		date = (pp->data[1] << 8 | pp->data[0]);
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
report(uchar_t rtype)
{
	printf("<A%d:%d:", radio.my_node_id, rtype);
	if (rtype == RADIO_CMD_FIRMWARE) {
		printf("%d,%d,%d.\n", NODE_TYPE, FW_VERSION_H, FW_VERSION_L);
	} else {
		printf("%d,%u,%u,%u,%u.\n", system_state, ticks, tens_of_minutes,
							radio.npacket_rx, radio.npacket_tx);
	}
}
