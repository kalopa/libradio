/*
 * Copyright (c) 2023, Kalopa Robotics Limited.  All rights reserved.
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
 * A simple frequency monitor - just report on whatever data is seen on
 * a given channel.
 */
#include <stdio.h>
#include <avr/io.h>
#include <avr/eeprom.h>

#include <libavr.h>
#include "libradio.h"
#include "../lib/internal.h"

#define FW_VERSION_H	0
#define FW_VERSION_L	1

struct channel	recvchan;

/*
 *
 */
int
main()
{
	int i;
	struct channel *chp = &recvchan;

	/*
	 * Timer1 is the workhorse. It is set up with a divide-by-64 to free-run
	 * (CTC mode) at 250kHz. We use OCR1A (set to 24999) to derive a timer
	 * frequency of 10Hz (0.625Hz in low power mode).
	 */
	_setled(1);
	TCNT1 = 0;
	OCR1A = 24999;
	TCCR1A = 0;
	power_mode(1);
	/*
	 * Set the baud rate and configure the USART. Our chosen baud rate is
	 * 38.4kbaud (with a 16MHz crystal).
	 */
	cli();
	UBRR0 = 25;
	UCSR0B = (1<<RXCIE0)|(1<<RXEN0)|(1<<TXEN0);
	UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);
	(void )fdevopen(sio_putc, sio_getc);
	sei();
	_setled(0);
	printf("\n\nRadio monitor v%d.%d.\n", FW_VERSION_H, FW_VERSION_L);
	/*
	 * Initialize the radio circuitry. First look for our unique ID in EEPROM
	 * and default to 0/1 if it's not there. Then call the libradio init
	 * function. Tell libradio that our high speed and low speed
	 * clock period is 100ms (or 10Hz).
	 */
	libradio_init(0, 0, 0, 0);
	libradio_set_clock(10, 10);
	libradio_irq_enable(1);
	radio.my_channel = radio.curr_channel = radio.my_node_id = 0;
	libradio_set_song(LIBRADIO_STATE_ACTIVE);
	/*
	 * Begin the main loop - every clock tick, call the radio loop.
	 */
	while (1) {
		/*
		 * Call the libradio function to see if there's anything to do. This
		 * routine only returns after a sleep or if the main clock has ticked.
		 */
		while (irq_fired == 0 && libradio_get_thread_run() == 0)
			_sleep();
		printf("I%d-%u\n", irq_fired, radio.ms_ticks);
		if (irq_fired) {
			libradio_get_int_status();
			libradio_irq_enable(1);
		}
		radio.main_ticks = 1000;
		if (libradio_recv(chp, radio.my_channel) == 0)
			continue;
		printf("Packet RX! (ch%d,tk:%u,len:%d)\n", radio.my_channel, radio.ms_ticks, chp->offset);
		printf("N%dL%dC%d", chp->payload[0], chp->payload[1], chp->payload[2]);
		for (i = 3; i < chp->offset; i++)
			printf(" 0x%x", chp->payload[i] & 0xff);
		putchar('\n');
	}
}

/*
 * Call-back function used to reduce power on the device and also slow down
 * the clock timer. Called when the system enters one of the two low power
 * states. In this case, the high speed clock has a period of 100ms (/64
 * divider) and the low speed clock has a period of 1.6s (/1024 divider).
 * This is also a good place to turn off unnecessary external hardware, if
 * possible. The timer will "glitch" in that the TCNT1 count is reset each
 * time this function is called, so it should only be called when appropriate.
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

/*
 *
 */
void
operate(struct packet *pp)
{
}

/*
 *
 */
int
fetch_status(uchar_t status_type, uchar_t status[], int maxlen)
{
	return(0);
}
