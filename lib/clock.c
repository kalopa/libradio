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

#define LED_NUMERATOR	16
#define LED_DENOMINATOR	125

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
	 * Configure TIMER0 for the real-time clock.
	 * Set Timer0 to CTC Mode (Mode 2) running at 125Hz
	 * WGM02 = 0, WGM01 = 1, WGM00 = 0
	 * OCR0A = 124
	 * CS02=1, CS01=0, CS00=1 -> Prescaler 1024
	 * Enable the Output Compare Match A Interrupt
	 */
	OCR0A = 124;
	TCCR0A = (1 << WGM01);
	TCCR0B = ((1 << CS02) | (1 << CS00));
	TIMSK0 = (1 << OCIE0A);
	/*
	 * Configure TIMER1 for our packet transmissions.
	 * WGM13:10 = 0 (Normal Mode)
	 * CS12=1, CS11=0, CS10=0 -> Prescaler 256
	 * Interrupt on Compare A Match
	 */
	TCCR1A = 0;
	TCCR1B = (1 << CS12);
	TCNT1 = 0;
	OCR1A = 62500;		/* 1 second slots at 16MHz/256 */
	TIMSK1 = (1 << OCIE1A);
	/*
	 * Configure TIMER2 for the servo output.
	 */
	//ASSR &= ~(1 << AS2); 
}

/*
 * This is called every 8ms by the TIMER2 interrupt system. Just update
 * the time of day, blink the LED and any timers which might have expired.
 */
void
clocktimer()
{
	/*
	 * millis counts the milliseconds and seconds whereas hhmm counts
	 * the minutes since midnight.
	 */
	if ((radio.millis += 8) >= 60000) {
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
