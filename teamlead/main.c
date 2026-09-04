/*
 * Copyright (c) 2020-25, Kalopa Robotics Limited.  All rights reserved.
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
 * of its active channels at regular intervals.
 */
#include <stdio.h>
#include <avr/io.h>
#include <string.h>

#include <libavr.h>

#include "libradio.h"
#include "teamlead.h"

/*
 * Get the ball rolling on the main controller.
 */
int
main()
{
 	/*
	 * Do the device initialization first...
	 */
	cli();
	_setled(1);
	serial_init();
	sei();
	printf("MCUSR%x\n", MCUSR);
	MCUSR = 0;
	printf("\nMain radio control system v%d.%d.\n",
					FW_VERSION_H, FW_VERSION_L);
	/*
	 * Initialize the radio circuitry. We don't really care about our category
	 * and ID number but we have to pass them for completeness.
	 */
	libradio_init(CONTROL_C1, CONTROL_C2, CONTROL_N1, CONTROL_N2);
	libradio_command_callback(rxdata);
	libradio_slot_callback(slot_event);
	libradio_teamlead();
	//sio_set_direct_mode(1);
	/*
	 * Begin the main loop - every clock tick, call the radio loop.
	 */
	while (1) {
		/*
		 * Wait for something to happen. The bulk of the work is done in the
		 * library code, and we provide callbacks for command handling and slot
		 * transmission events.
		 */
		libradio_poll();
		/*
		 * Handle serial data from our upstream overlords.
		 */
		if (!sio_iqueue_empty());
			//process_input();
	}
}
