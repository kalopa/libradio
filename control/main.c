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
 * not the radio interface.
 */
#include <stdio.h>
#include <avr/io.h>
#include <string.h>

#include <libavr.h>

#include "libradio.h"
#include "control.h"

/*
 * Get the ball rolling on the main controller. Note that we start up in a
 * LISTEN state too, but it's mostly irrelevant. We don't really go into the
 * same low-power, suspended animation state of the clients. We'll be
 * checking for RS232 data pretty much always, even in a COLD sleep. The
 * assumption is that power consumption isn't an issue, and the upstream
 * comms client won't want to wait an hour until the device sees the message.
 */
int
main()
{
 	/*
	 * Do the device initialization first...
	 */
	cli();
	_setled(1);
	clock_init();
	serial_init();
	sei();
	printf("\nMain radio control system v%d.%d.\n",
					FW_VERSION_H, FW_VERSION_L);
	local_status(RADIO_STATUS_DYNAMIC);
	local_status(RADIO_STATUS_STATIC);
	/*
	 * Initialize the radio circuitry. We don't really care about our category
	 * and ID number because we will get activated over the 1:1 serial line.
	 * The EEPROM data however will be used for channel ownership information.
	 */
	libradio_init(CONTROL_C1, CONTROL_C2, CONTROL_N1, CONTROL_N2);
	tx_init();
	/*
	 * Begin the main loop - every clock tick, call the radio loop.
	 */
	libradio_set_state(LIBRADIO_STATE_WARM);
	while (1) {
		while (libradio_get_thread_run() == 0) {
			/*
			 * Handle serial data from our upstream overlords.
			 */
			if (!sio_iqueue_empty())
				process_input();
			_sleep();
		}
		/*
		 * Run down through the list of channels and see if anything needs
		 * to be transmitted. This is based on the priority.
		 */
		tx_check_queues();
	}
}

/*
 * These are required callbacks but they don't do anything.
 *
 * Handle a non-default command.
 */
void
operate(struct packet *pp)
{
}

/*
 * Fetch the status block and report it back.
 */
int
fetch_status(uchar_t status_type, uchar_t *cp, int maxlen)
{
	return(0);
}
