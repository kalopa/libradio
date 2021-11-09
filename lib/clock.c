/*
 * Copyright (c) 2020-21, Kalopa Robotics Limited.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ABSTRACT
 * As the date and time as well as the state machine are intimately
 * intertwined with the real-time clock interrupts, the actual ISR is
 * part of this library rather than the application. It also manages to
 * flash the main LED with the appropriate indication of current state.
 */
#include <stdio.h>
#include <avr/io.h>

#include <libavr.h>

#include "libradio.h"
#include "internal.h"

/*
 * LED "songs" for the first six states.
 */
uint_t	songs[NLIBRADIO_STATES] = {0xffff, 0x5555, 0x1, 0x3, 0xff, 0xf7};

/*
 * This function is called every clock tick (every 10ms) or ten times
 * per second. Increment the millisecond clock, and if we've passed
 * the tens of minutes, then increment that. The ToM parks itself at
 * 145 which is an illegal value used at init, to let us know we don't
 * actually know what time it is. We also call the heartbeat to flash
 * the LED appropriately. This should really be called every 62.5ms.
 * We use main_thread so the main loop doesn't run too quickly.
 */
void
clocktick()
{
	uchar_t ledf;

	/*
	 * Only track the time of day if we've been given some sort
	 * of useful date value via external comms.
	 */
	if (radio.tens_of_minutes != 0xff) {
		if ((radio.ms_ticks += radio.fast_period) >= 60000) {
			radio.ms_ticks = 0;
			radio.tens_of_minutes++;
		}
	}
	/*
	 * Update the main LED song, one note at a time...
	 */
	_setled(ledf = (radio.heart_beat & 0x8000) ? 1 : 0);
	radio.heart_beat = (radio.heart_beat << 1) | ledf;
	/*
	 * Kick the main thread, if appropriate.
	 */
	if (radio.main_ticks != 0 && --radio.main_ticks == 0) {
		radio.main_thread = 1;
		radio.main_ticks = 1;
	}
}

/*
 * Set the LED "song" based on the state.
 */
void
libradio_set_song(uchar_t new_state)
{
	if (new_state <= LIBRADIO_STATE_ACTIVE)
		radio.heart_beat = songs[new_state];
	else
		radio.heart_beat = songs[LIBRADIO_STATE_ACTIVE];
}
