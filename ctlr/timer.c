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
 * All the clock timer functionality for the main controller is embedded
 * in this file. The actual interrupt handler is in the library.
 */
#include <stdio.h>
#include <avr/io.h>

#include <libavr.h>

#include "libradio.h"

uint_t	songs[NRADIO_STATES] = {0xfff, 0, 0, 0xf, 0xf7, 0x5555};

volatile uint_t		delay_ticks;
volatile char		hbticks;
volatile uint_t		hbeat;

volatile uint_t		ticks;
volatile uchar_t	tens_of_minutes;
uchar_t			system_state;
uint_t			date;
uchar_t			main_thread;

/*
 * Initialize the time of day clock system. Here's the run-down of the usage:
 * Timer0 - used for RTC interrupts. Set at 1/64 divider, and CTC mode, with
 * OCR0A set to 250 (for a clock frequency of 1kHz).
 */
void
clockinit()
{
	/*
	 * Set up our notion of time and timers.
	 */
	ticks = 0;
	hbticks = 0;
	tens_of_minutes = 145;
	date = 0;
	delay_ticks = 0;
	main_thread = 0;
	/*
	 * Timer1 is the workhorse. It is set up with a divide-by-64 to free-run
	 * (CTC mode) at 250kHz. We use OCR1A (set to 2499) to derive a timer
	 * frequency of 100Hz for the main clocktick() function below. We use
	 * the ICP1 input to latch the TCNT1 count into ICR1 whenever the wheel
	 * encoder registers a pulse. This is used for the motor control PID.
	 * Timer1 is also used for the servo control. We use OCR1B to output a
	 * variable pulse in steps of 4uS. For example, to get a 2ms pulse, we set
	 * OCR1B to 500. For a 1ms pulse, we set it to 250.
	 */
	TCNT1 = 0;
	OCR1A = 2499;
	TCCR1A = 0;
	TCCR1B = (1<<ICNC1)|(1<<CS11)|(1<<CS10);
	TCCR1C = 0;
	TIMSK1 = (1<<ICIE1)|(1<<OCIE1A);
	/*
	 * Set up Timer0 for our motor speed control. We run the clock in PWM,
	 * Phase Correct mode. TOP is OCR0A. Clear OC0A on Compare-Match when
	 * up-counting. Set OC0A on Compare-Match when down-counting.
	 * No pre-scale.
	 */
	TCNT0 = 0;
	OCR0A = 0;
	TCCR0A = (1<<COM0A1)|(1<<WGM01)|(1<<WGM00);
	TCCR0B = (1<<CS00);
	TIMSK0 = 0;
}

/*
 * This function is called every clock tick (every millisecond).
 */
void
clocktick()
{
	uchar_t ledf;

	OCR1A += 2500;
	main_thread = 1;
	if (++ticks >= 60000) {
		ticks = 0;
		if (tens_of_minutes < 145)
			tens_of_minutes++;
	}
	if (delay_ticks > 0)
		delay_ticks--;
	if ((hbticks -= 4) <= 0) {
		hbticks += 25;
		ledf = (hbeat & 0x8000) ? 1 : 0;
		_setled(ledf);
		hbeat = (hbeat << 1) | ledf;
	}
}

/*
 *
 */
void
delay(int nticks)
{
	delay_ticks = nticks;

	while (delay_ticks > 0)
		_watchdog();
}

/*
 * Set the operational state (and as a side-effect, also set the LED "song").
 */
void
set_state(uchar_t new_state)
{
	system_state = new_state;
	hbeat = songs[system_state];
}
