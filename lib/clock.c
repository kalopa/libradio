/*
 * Copyright (c) 2020-25, Kalopa Robotics Limited.  All rights reserved.
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
 * The main library initialization code. The init call takes four bytes as
 * arguments. These represent the "world"-unique identification numbers
 * for the devices. They don't actually need to be globally unique,
 * just unique to your environment. The first two bytes represent
 * the category of device, whereas the last two represent the device
 * number. Sort of like the ClassID and the InstanceID. Define a taxonomy
 * for your environment, and assign the cat1 and cat2 bytes based on
 * that schema. Then number each unique device within each category,
 * so that no two devices ever have the same four-byte number.
 */
#include <stdio.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <string.h>

#include <libavr.h>

#include "libradio.h"
#include "internal.h"

/*
 * The LED "song" steps 16 times a second. We're called once per 20ms slot
 * (50 times a second), so we step the song on a 16/50 rational count.
 */
#define LED_NUMERATOR	16
#define LED_DENOMINATOR	50

/*
 * LED "songs" for the first six states.
 */
uint_t		songs[LIBRADIO_NSTATES] = {0, 0x00ff, 0x3333, 0xf7, 0xf7};

/*
 * :_ticks is used by the ISR to only call the C code every ten clock
 * ticks (running at 1kHz this means clocktimer is called every 10ms
 * or 100Hz).
 * :hbclock attempts to get the LED to play every 62.5ms. We use rational
 * numbers to achieve this.
 */
uchar_t		hbit;
uchar_t		hbclock;
uint_t		song;

/*
 * Initialize our clocks.
 */
void
clock_init()
{
	hbit = hbclock = 0;
	radio.millis = radio.hhmm = 0;
	/*
	 * Configure TIMER1 as the TDMA slot clock.
	 * WGM13:10 = 0 (Normal Mode - free-running, we advance OCR1A by
	 * SLOT_TICKS ourselves at each boundary; see libradio_slot_advance())
	 * CS11=1, CS10=1 -> Prescaler 64 (250kHz @ 16MHz, 4us resolution)
	 * Interrupt on Compare A Match, every SLOT_TICKS (20ms)
	 *
	 * We deliberately leave TIMER0 and TIMER2 alone. TIMER0 in particular
	 * must stay free for the application: the garden railway locomotive
	 * drives its motor with hardware PWM on OC0A (PD6) and can't move to
	 * another timer without rewiring the board. Everything we used to do
	 * with TIMER0 (the real-time clock and the LED song) now hangs off
	 * the 20ms slot tick instead - 50 slots is exactly one second, so the
	 * seconds/minutes counters come out exact.
	 */
	TCCR1A = 0;
	TCCR1B = ((1 << CS11) | (1 << CS10));
	TCNT1 = 0;
	OCR1A = SLOT_TICKS;
	TIMSK1 = (1 << OCIE1A);
}

/*
 * Called once per 20ms slot boundary (see libradio_slot_advance()). Just
 * update the time of day and blink the LED.
 */
void
clocktimer()
{
	/*
	 * millis counts the milliseconds and seconds whereas hhmm counts
	 * the minutes since midnight. 50 slots is exactly one second, so
	 * 3000 slots is exactly one minute - no rounding error to worry about.
	 */
	if ((radio.millis += SLOT_MS) >= 60000) {
		radio.millis = 0;
		if (++radio.hhmm > (24*60))
			radio.hhmm = 0;
	}
	/*
	 * Update the LED status - play the song.
	 */
	if ((hbclock += LED_NUMERATOR) >= LED_DENOMINATOR) {
		uchar_t ledf;

		hbclock -= LED_DENOMINATOR;
		if (hbit-- == 0) {
			hbit = 15;
			song = songs[radio.state];
		}
		_setled(ledf = (song & 0x8000) ? 1 : 0);
		song = (song << 1) | ledf;
	}
}

/*
 * Return the current millisecond counter (0-59999, resets every minute -
 * see clocktimer() above). Fine for short, sub-minute timing; for
 * anything longer use libradio_hhmm() instead.
 */
uint_t
libradio_millis()
{
	return(radio.millis);
}

/*
 * Return the minutes-since-midnight counter (0-1439). Apps use this to
 * time their own longer periodic work (e.g. "check the sensor every 5
 * minutes") off the free-running clock, rather than libradio owning any
 * notion of what the app should do and when.
 */
uint_t
libradio_hhmm()
{
	return(radio.hhmm);
}
