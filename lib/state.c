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
 * The function to set the system state lives here. So all of the
 * mechanics of the state machine are defined in here.
 */
#include <stdio.h>
#include <avr/io.h>

#include <libavr.h>

#include "libradio.h"
#include "internal.h"

/*
 * Helper function to return the current system state. This is not the
 * state of the radio, or the state of a particular channel packet,
 * but the overall state of operation. Returning, for example,
 * LIBRADIO_STATE_ACTIVE.
 */
uchar_t
libradio_get_state()
{
	return(radio.state);
}

/*
 * What channel am I on? Return the saved value of what channel the
 * radio thinks we're on.
 */
uchar_t
libradio_get_my_channel()
{
	return(radio.curr_channel);
}

/*
 * Update the state and the associated parameters. Note that this should
 * be idempotent in that it will not change anything unless there is an
 * actual state change.
 */
void
libradio_set_state(uchar_t new_state)
{
	long ticks;

	if (new_state != LIBRADIO_STATE_STARTUP && radio.state == new_state)
		return;
	libradio_set_song(new_state);
	switch (new_state) {
	case LIBRADIO_STATE_COLD:
	case LIBRADIO_STATE_WARM:
		/*
		 * Time to reduce power and wait for a while. Also turn off the real
		 * time clock - no point trying to track the time in this mode. Wait
		 * for 5 or 60 minutes depending.
		 */
		libradio_power_mode(0);
		radio.tens_of_minutes = 0xff;
		ticks = (new_state == LIBRADIO_STATE_WARM) ? 5*60*100L : 60*60*100L;
		if ((ticks /= (long )radio.period) > 65535L)
			ticks = 65535L;
		libradio_set_delay((int )ticks);
		break;

	case LIBRADIO_STATE_LISTEN:
		/*
		 * Listen for radio activity for one minute.
		 */
		if (radio.state == LIBRADIO_STATE_COLD ||
						radio.state == LIBRADIO_STATE_WARM) {
			/*
			 * Go into high power mode while in this state. Just make sure
			 * that we were already in low power mode, or we'll glitch the
			 * real-time clock and the time of day.
			 */
			libradio_power_mode(1);
		}
		/*
		 * Switch to channel 0, clear the packet flag, and set a timeout
		 * for sixty seconds. We need to see something in that period,
		 * or else we're going for a nice, long sleep...
		 */
		radio.my_channel = 0;
		radio.saw_rx = 0;
		if (radio.state == LIBRADIO_STATE_COLD)
			radio.timeout = 6000 / radio.fast_period;
		else
			radio.timeout = (int )(12000L / (long )radio.fast_period);
		libradio_set_delay(500);
		break;

	case LIBRADIO_STATE_ERROR:
		/*
		 * Some sort of fatal error. Stay in this state for a minute.
		 */
		libradio_set_delay((int )(60*100L / (long )radio.period));
		break;

	default:
		/*
		 * All of the active states. The thing here is to rewind the RX
		 * timeout for 60,000 passes through the loop (whatever that is
		 * in seconds).
		 */
		libradio_power_mode(1);
		radio.timeout = 6000;
		break;
	}
	radio.state = new_state;
}
