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
uint_t		songs[NLIBRADIO_STATES] = {0xffff, 0x3333, 0x0, 0x1, 0xf, 0xff, 0xf7};

uchar_t		hbclock = 0;
uint_t		timer_count;
uchar_t		thread_ok;
uchar_t		elapsed_second;
uchar_t		irq_fired;

/*
 * This function is called every clock tick (every 10ms) or one hundred
 * times per second. Increment the millisecond clock, and if we've passed the
 * tens of minutes, then increment that. The ToM parks itself at 145 which is
 * an illegal value used at init, to let us know we don't actually know what
 * time it is. We also call the heartbeat to flash the LED appropriately.
 * This should really be called every 62.5ms. We use main_thread so the main
 * loop doesn't run too quickly.
 */
void
clocktick()
{
	uchar_t ledf;

	/*
	 * Update the main LED song, one note at a time. The whole thing should
	 * take a consistent 1.6 seconds.
	 */
	if ((hbclock += radio.period) > 100) {
		hbclock -= 100;
		_setled(ledf = (radio.heart_beat & 0x8000) ? 1 : 0);
		radio.heart_beat = (radio.heart_beat << 1) | ledf;
	}
	/*
	 * Only track the time of day if we've been given some sort
	 * of useful date value via external comms.
	 */
	if (radio.tens_of_minutes != 0xff) {
		if ((radio.ms_ticks += radio.period) >= 60000) {
			radio.ms_ticks = 0;
			radio.tens_of_minutes++;
		}
	}
	/*
	 * Kick the main thread, if appropriate.
	 */
	if (radio.main_ticks != 0 && --radio.main_ticks == 0) {
		thread_ok = 1;
		radio.main_ticks = 1;
	}
	/*
	 * Increment the seconds counter in case the main loop needs to do
	 * periodic work (like check the battery).
	 */
	if ((timer_count += radio.period) >= 1000) {
		timer_count = 0;
		elapsed_second = 1;
	}
}

/*
 * Enable radio IRQs. These appear on PD2 (INT0) and simply disable any
 * further interrupts and set irq_fired, which is used by the wait-timer.
 * We use catch_irq to track the fact we're using radio IRQs. This is so
 * we remember to re-enable them, later on (see libradio_recv()).
 */
void
libradio_irq_enable(uchar_t flag)
{
	irq_fired = 0;
	EICRA = 0;
	if ((radio.catch_irq = flag) == 0)
		EIMSK = 0;
	else
		EIMSK = 01;
}

/*
 * Wait for the main tick to occur. Called from main code (not ISR).
 */
int
libradio_get_thread_run()
{
	int old_value = thread_ok;

	thread_ok = 0;
	return(old_value);
}

/*
 * Wait for a thread run.
 */

/*
 * Check to see if the timer has fired.
 */
int
libradio_elapsed_second()
{
	uchar_t old_value = elapsed_second;

	elapsed_second = 0;
	return(old_value);
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
