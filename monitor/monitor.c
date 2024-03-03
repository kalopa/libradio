/*
 * Copyright (c) 2024, Kalopa Robotics Limited.  All rights reserved.
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
 * Prototypes
 */
void		display_packet(struct channel *);
void		hexdigit(char);

FILE		iodev;

void
mysetup()
{
	FILE *s;

	s = &iodev;
	s->flags = __SMALLOC;
	s->get = sio_getc;
	s->put = sio_putc;
	s->flags = (__SMALLOC|__SRD|__SWR);
	stdin = s;
	stdout = s;
	stderr = s;
}

/*
 *
 */
int
main()
{
	int irqf;
	struct channel *chp = &recvchan;

	/*
	 * Timer1 is the workhorse. It is set up with a divide-by-64 to free-run
	 * (CTC mode) at 250kHz. We use OCR1A (set to 2499) to derive a timer
	 * frequency of 100Hz.
	 */
	_setled(1);
	cli();
	TCNT1 = 0;
	OCR1A = 2499;
	TCCR1A = 0;
	TCCR1B = (1<<WGM12)|(1<<CS11)|(1<<CS10);
	TCCR1C = 0;
	TIMSK1 = (1<<OCIE1A);
	/*
	 * Set the baud rate and configure the USART. Our chosen baud rate is
	 * 38.4kbaud (with a 16MHz crystal).
	 */
	UBRR0 = 25;
	UCSR0B = (1<<RXCIE0)|(1<<RXEN0)|(1<<TXEN0);
	UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);
	//(void )fdevopen(sio_putc, sio_getc);
	mysetup();
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
	libradio_set_clock(1, 1);
	libradio_irq_enable(1);
	radio.my_channel = radio.curr_channel = 0;
	libradio_recv_start();
	radio.my_node_id = 0;
	libradio_set_state(LIBRADIO_STATE_ACTIVE);
	libradio_set_delay(100);
	/*
	 * Begin the main loop - every clock tick, call the radio loop.
	 */
	printf("Listening on channel %d.\n", radio.my_channel);
	while (1) {
		/*
		 * Call the libradio function to see if there's anything to do. This
		 * routine only returns after a sleep or if the main clock has ticked.
		 */
		irqf = libradio_wait();
		if (!sio_iqueue_empty())
			libradio_debug();
		if (libradio_elapsed_second() && radio.tens_of_minutes > 143)
			radio.tens_of_minutes = 0;
		if (irqf) {
			libradio_get_fifo_info(0);
			libradio_get_int_status();
			libradio_get_modem_status();
			libradio_irq_enable(1);
		}
		/*
		 * If we have a packet, print it out.
		 */
		while (radio.rx_fifo >= SI4463_PACKET_LEN) {
			if (libradio_recv(chp, radio.my_channel))
				display_packet(chp);
			libradio_get_fifo_info(0);
		}
		_watchdog();
	}
}

/*
 * Display the contents of a single packet.
 */
void
display_packet(struct channel *chp)
{
	int i, len, value;
	uchar_t *cp;

	printf("RX%d,%d/%d:", radio.my_channel, radio.current_rssi, radio.latch_rssi);
	if (chp->packet.node > 5)
		printf("N?");
	if (chp->packet.cmd > 16)
		printf("C?");
	if ((len = chp->packet.len) > MAX_PAYLOAD_SIZE) {
		printf("L?");
		len = MAX_PAYLOAD_SIZE;
	}
	len += PACKET_HEADER_LEN;
	for (i = 0, cp = (uchar_t *)&chp->packet; i < len; i++) {
		value = *cp++;
		putchar((i == 6) ? '.' : ' ');
		hexdigit((value >> 4) & 15);
		hexdigit(value & 15);
	}
	putchar('\n');
}

/*
 * In this case, we never take our foot off the gas.
 */
void
libradio_power_mode(uchar_t hi_flag)
{
	return;
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

/*
 * Print a hex digit.
 */
void
hexdigit(char ch)
{
	if (ch > 9)
		ch += 7;
	putchar(ch + '0');
}
