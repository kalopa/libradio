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
 * Initialize things (like the clock and the serial I/O).
 */
#include <stdio.h>
#include <avr/io.h>
#include <string.h>

#include <libavr.h>

#include "libradio.h"
#include "control.h"

/*
 * Initialize the time of day clock system. Here's the run-down of the usage:
 * Timer0 - used for RTC interrupts. Set at 1/64 divider, and CTC mode, with
 * OCR0A set to 250 (for a clock frequency of 1kHz).
 */
void
clock_init()
{
	/*
	 * Timer1 is the workhorse. It is set up with a divide-by-64 to free-run
	 * (CTC mode) at 250kHz. We use OCR1A (set to 2499) to derive a timer
	 * frequency of 100Hz for the main clocktick() function. If we were in
	 * fact using the power-down mode, we'd choose a 1/8 divider and an OCR1A
	 * value of 19999. But there's no low power mode, so...
	 */
	TCNT1 = 0;
	OCR1A = 2499;
	TCCR1A = 0;
	TCCR1B = (1<<WGM12)|(1<<CS11)|(1<<CS10);
	TCCR1C = 0;
	TIMSK1 = (1<<OCIE1A);
}

/*
 * Initialize the serial port.
 */
void
serial_init()
{
	/*
	 * Set the baud rate and configure the USART. Our chosen baud rate is
	 * 38.4kbaud (with a 16MHz crystal).
	 */
	UBRR0 = 25;
	UCSR0B = (1<<RXCIE0)|(1<<UDRIE0)|(1<<RXEN0)|(1<<TXEN0);
	UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);
	//sio_set_direct_mode(1);
	(void )fdevopen(sio_putc, sio_getc);
}

/*
 * Set low or high power mode, accordingly.
 */
void
power_mode(uchar_t hi_flag)
{
	cli();
	/*
	 * Fast has CS1n = 3 (/64) and slow CS1n = 5 (/1024).
	 */
	if (hi_flag)
		TCCR1B = (1<<WGM12)|(1<<CS11)|(1<<CS10);
	else
		TCCR1B = (1<<WGM12)|(1<<CS12)|(1<<CS10);
 	TCCR1C = 0;
	TIMSK1 = (1<<OCIE1A);
	sei();
}
