/*
 * Copyright (c) 2020-25, Kalopa Robotics Limited.  All rights reserved.
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

/*
 * Milestone 1: no dynamic channel assignment yet, so this device's TDMA
 * slot is hardcoded. Once assignment exists (Milestone 2, via teamlink),
 * this goes away in favour of whatever the team lead grants.
 */
#define MY_SLOT				1

void	command(struct packet *);
void	slot_event(int);
void	get_battery_voltage();
void	get_oil_level();

uchar_t		mynum1;
uchar_t		mynum2;
int		battery_voltage;
int		oil_level;

/*
 *
 */
int
main()
{
	uint_t next_check;

	_setled(1);
	cli();
	/*
	 * Note: don't set up Timer0 or Timer1 here - libradio owns both (see
	 * libradio.h). libradio_init() below configures Timer1 as the 20ms
	 * TDMA slot clock and Timer0 as the real-time clock, so anything we
	 * set up here would just be clobbered.
	 *
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
	libradio_command_callback(command);
	libradio_slot_callback(slot_event);
	/*
	 * Begin the main loop - every clock tick, call the radio loop.
	 */
	get_battery_voltage();
	get_oil_level();
	next_check = (libradio_hhmm() + 5) % (24 * 60);
	while (1) {
		/*
		 * Non-blocking - never waits. The bulk of the work (slot timing,
		 * packet TX/RX) happens in the library; we provide callbacks for
		 * command handling and slot transmission events.
		 */
		libradio_poll();
		/*
		 * Every five minutes, check the battery and the oil level.
		 */
		if (libradio_hhmm() == next_check) {
			get_battery_voltage();
			get_oil_level();
			next_check = (next_check + 5) % (24 * 60);
		}
	}
}

/*
 * As we don't have any custom command functions, this function just prints
 * whatever custom command was received. Useful for debugging, I suppose...
 */
void
command(struct packet *pp)
{
	printf("RX CMD: %x\n", pp->command);
}

/*
 * Called by libradio once per slot, LIBRADIO_TX_LEAD_TICKS before that
 * slot's boundary - well ahead of time, so there's no scramble to go
 * measure the tank right when it's our turn to transmit. We already have
 * battery_voltage/oil_level on hand (refreshed every five minutes above),
 * so just stage the status packet.
 */
void
slot_event(int slotid)
{
	static struct packet response;

	if (slotid != MY_SLOT)
		return;
	response.rxchan = LIBRADIO_CHANNEL_TEAMLEAD;
	response.command = RADIO_CMD_STATUS_RESPONSE;
	response.data[0] = OILTANK_CAT1;
	response.data[1] = OILTANK_CAT2;
	response.data[2] = mynum1;
	response.data[3] = mynum2;
	response.data[4] = FW_VERSION_H;
	response.data[5] = FW_VERSION_L;
	response.data[6] = (battery_voltage >> 8) & 0xff;
	response.data[7] = (battery_voltage & 0xff);
	response.data[8] = (oil_level >> 8) & 0xff;
	response.data[9] = (oil_level & 0xff);
	libradio_transmit(&response);
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
