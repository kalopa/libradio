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
 * The main client application just calls libradio_rxloop() in a while loop
 * and this does the bulk of the radio communications work. It manages the
 * state machine for the radio, and attempts to read (and process) any
 * received packets.
 */
#include <stdio.h>
#include <avr/io.h>

#include <libavr.h>

#include "libradio.h"
#include "internal.h"

struct channel	recvchan;

/*
 * This is called repeatedly from the main loop. The task here is based on
 * the current state. Note we will halt the processor if there is nothing
 * to do at this point.
 */
void
libradio_rxloop()
{
	/*
	 * First things first - wait for the clock timer to kick us into doing
	 * something. The sleep() function will pause the CPU until the next IRQ.
	 */
	libradio_wait_thread();
	/*
	 * Depending on what state we're in, do something useful. For a lot of
	 * these states, not much happens and all we do is move to the next
	 * state.
	 */
	switch (radio.state) {
	case LIBRADIO_STATE_ERROR:
		/*
		 * We are in an error state, for a full minute. Go to a WARM sleep.
		 */
		libradio_set_state(LIBRADIO_STATE_WARM);
		break;

	case LIBRADIO_STATE_STARTUP:
	case LIBRADIO_STATE_COLD:
	case LIBRADIO_STATE_WARM:
		/*
		 * Time to wake up and check for radio traffic.
		 */
		libradio_set_state(LIBRADIO_STATE_LISTEN);
		break;

	case LIBRADIO_STATE_LISTEN:
		libradio_handle_packet(&recvchan);
#if 0
		if (--radio.timeout == 0) {
			/*
			 * If we're still in this state after the timeout, then we go to
			 * WARM or COLD sleep depending on whether we've seen any traffic.
			 */
			libradio_set_state(radio.saw_rx ? LIBRADIO_STATE_WARM :
												LIBRADIO_STATE_COLD);
		}
#else
		radio.main_ticks = 10;
#endif
		break;

	default:
		/*
		 * Any active state - check for radio traffic.
		 */
		libradio_handle_packet(&recvchan);
		if (radio.saw_rx) {
			radio.timeout = 60000;
			radio.saw_rx = 0;
		}
		if (--radio.timeout == 0) {
			/*
			 * Haven't seen any traffic in a *long* time.
			 */
			libradio_set_state(LIBRADIO_STATE_WARM);
		}
		break;
	}
}
