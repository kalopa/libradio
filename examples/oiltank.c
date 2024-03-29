/*
 * Copyright (c) 2020-24, Kalopa Robotics Limited.  All rights reserved.
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
 * The main C code for the oil tank example. This is a simple reference
 * program for the library. It has an ultrasonic sensor attached, and
 * is designed to measure the height of liquid in a central heating oil
 * tank. It's a good reference as it only provides one simple function -
 * ping the oil level and report back the height. All of the libradio
 * functioality happens elsewhere. This just provides the timer config
 * (because we run a bit slower than normal - 10Hz rather than 100Hz).
 *
 * The operate() function is a call-back from the libradio code whenever
 * we get a request to do something. In this case, measure the oil!
 */
#include <stdio.h>
#include <avr/io.h>
#include <avr/eeprom.h>

#include <libavr.h>
#include "libradio.h"

#define FW_VERSION_H	1
#define FW_VERSION_L	5

/*
 * The two 8-bit codes define an overall category of device and a subcategory
 * but really they're just two bytes which need to be unique for this class
 * of thing. Neither should be 0xff.
 */
#define OILTANK_CAT1		0x7f				/* Monitoring device */
#define OILTANK_CAT2		0x01				/* Tank level monitor */

void	get_battery_voltage();
void	get_oil_level();

uchar_t		mynum1;
uchar_t		mynum2;
int			battery_voltage;
int			oil_level;

/*
 *
 */
int
main()
{
	int tlsecs;

	_setled(1);
	cli();
	/*
	 * Timer1 is the workhorse. It is set up with a divide-by-64 to free-run
	 * (CTC mode) at 250kHz. We use OCR1A (set to 24999) to derive a timer
	 * frequency of 10Hz (100ms per tick) and 0.625Hz in low power mode.
	 */
	TCNT1 = 0;
	OCR1A = 24999;
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
	(void )fdevopen(sio_putc, sio_getc);
	sei();
	_setled(0);
	printf("\n\nOil tank monitor v%d.%d.\n", FW_VERSION_H, FW_VERSION_L);
	/*
	 * Initialize the radio circuitry. First look for our unique ID in EEPROM
	 * and default to 0/1 if it's not there. Then call the libradio init
	 * function. Tell libradio that our high speed clock period is 100ms (or
	 * 10Hz) rather than the usual default of 10ms, and our low-power clock
	 * period is 1.6s.
	 */
	mynum1 = eeprom_read_byte((const unsigned char *)0);
	mynum2 = eeprom_read_byte((const unsigned char *)1);
	if (mynum1 == 0xff || mynum2 == 0xff) {
		mynum1 = 0;
		mynum2 = 1;
		eeprom_write_byte((unsigned char *)0, mynum1);
		eeprom_write_byte((unsigned char *)1, mynum2);
	}
	printf("IDENT %d/%d/%d/%d\n", OILTANK_CAT1, OILTANK_CAT2, mynum1, mynum2);
	libradio_init(OILTANK_CAT1, OILTANK_CAT2, mynum1, mynum2);
	libradio_set_clock(10, 160);
	libradio_recv_start();
	libradio_irq_enable(1);
	/*
	 * Begin the main loop - every clock tick, call the radio loop.
	 */
	tlsecs = 0;
	get_battery_voltage();
	get_oil_level();
	while (1) {
		/*
		 * Call the libradio function to see if there's anything to do. This
		 * routine only returns after a sleep or if the main clock has ticked.
		 */
		libradio_rxloop();
		/*
		 * Every five minutes, check the battery and the oil level.
		 */
		if (libradio_elapsed_second() && ++tlsecs >= 300) {
			get_battery_voltage();
			get_oil_level();
			tlsecs = 0;
		}
	}
}

/*
 * As we don't have any custom command functions, this function just prints
 * whatever custom command was received. Useful for debugging, I suppose...
 */
void
operate(struct packet *pp)
{
	printf("Oil Tank OPERATE function.\n");
	printf("Command: %d, len: %d\n", pp->cmd, pp->len);
}

/*
 * Report oiltank status back to the requested receiver.
 */
int
fetch_status(uchar_t status_type, uchar_t status[], int maxlen)
{
	if (maxlen < 7)
		return(0);
	status[0] = status_type;
	switch (status_type) {
	case RADIO_STATUS_STATIC:
		/*
		 * Status which doesn't really change.
		 */
		status[1] = OILTANK_CAT1;
		status[2] = OILTANK_CAT2;
		status[3] = mynum1;
		status[4] = mynum2;
		status[5] = FW_VERSION_H;
		status[6] = FW_VERSION_L;
		return(7);

	case RADIO_STATUS_DYNAMIC:
		/*
		 * Dynamic status.
		 */
		status[1] = libradio_get_state();
		status[2] = (battery_voltage >> 8) & 0xff;
		status[3] = (battery_voltage & 0xff);
		status[4] = (oil_level >> 8) & 0xff;
		status[5] = (oil_level & 0xff);
		return(6);
	}
	return(0);
}

/*
 * Return the battery voltage as an integer between 0 and 1023. This is
 * computed by reading the A/D value after the voltage divider circuit.
 */
void
get_battery_voltage()
{
	battery_voltage = analog_read(3);
}

/*
 * Read the oil tank level. This sensor is comprised of a one-shot ultrasonic
 * sensor which "pings" the oil level from above, and measures the time to a
 * response. With a rough approximation of the speed of sound in air, it is
 * possible to compute the oil level in the tank. Note that this function
 * just returns the raw sample data and some higher-level code will try and
 * figure out the actual level.
 */
void
get_oil_level()
{
	oil_level = 800;
	printf("Oil=%d\n", oil_level);
}
