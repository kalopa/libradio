/*
 * Copyright (c) 2019-21, Kalopa Robotics Limited.  All rights reserved.
 * Unpublished rights reserved under the copyright laws  of the Republic
 * of Ireland.
 *
 * The software contained herein is proprietary to and embodies the
 * confidential technology of Kalopa Robotics Limited.  Possession,
 * use, duplication or dissemination of the software and media is
 * authorized only pursuant to a valid written license from Kalopa
 * Robotics Limited.
 *
 * RESTRICTED RIGHTS LEGEND   Use, duplication, or disclosure by
 * the U.S.  Government is subject to restrictions as set forth in
 * Subparagraph (c)(1)(ii) of DFARS 252.227-7013, or in FAR 52.227-19,
 * as applicable.
 *
 * THIS SOFTWARE IS PROVIDED BY KALOPA ROBOTICS LIMITED "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL KALOPA ROBOTICS LIMITED
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ABSTRACT
 * This is where it all kicks off. Have fun, baby!
 */
#include <stdio.h>
#include <avr/io.h>
#include <avr/eeprom.h>

#include <libavr.h>
#include "libradio.h"

#define BAUDRATE	25						/* 38.4kbaud @ 16Mhz */

volatile char		hbticks;
volatile uchar_t	main_thread;

struct libradio		radio;

/*
 *
 */
int
main()
{
	int ch, bs_state;

 	/*
	 * Do the low-level initialization, and then the upper-level stuff.
	 */
	hbticks = 0;
	main_thread = 0;
	/*
	 * Timer1 is the workhorse. It is set up with a divide-by-64 to free-run
	 * (CTC mode) at 250kHz. We use OCR1A (set to 24999) to derive a timer
	 * frequency of 10Hz for the main clocktick() function below.
	 */
	TCNT1 = 0;
	OCR1A = 24999;
	TCCR1A = 0;
	TCCR1B = (1<<WGM12)|(1<<CS11)|(1<<CS10);
 	TCCR1C = 0;
	TIMSK1 = (1<<OCIE1A);
	_setled(1);
	/*
	 * Set baud rate and configure the USART.
	 */
	UBRR0 = BAUDRATE;
	UCSR0B = (1<<RXCIE0)|(1<<UDRIE0)|(1<<RXEN0)|(1<<TXEN0);
	UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);
	(void )fdevopen(sio_putc, sio_getc);
	sei();
	printf("\nOil tank monitor v1.0. MCUSR:%x\n", MCUSR);
	/*
	 * Initialize the radio circuitry
	 */
	radio.unique_code1 = eeprom_read_byte((const unsigned char *)0);
	radio.unique_code2 = eeprom_read_byte((const unsigned char *)1);
	if (radio.unique_code1 == 0xff && radio.unique_code2 == 0xff) {
		radio.unique_code1 = 0x7f;
		radio.unique_code2 = 0x01;
		eeprom_write_byte((unsigned char *)0, radio.unique_code1);
		eeprom_write_byte((unsigned char *)1, radio.unique_code2);
	}
	libradio_init(&radio);
	/*
	 * Begin the main loop - every clock tick, call the radio loop.
	 */
	bs_state = 0;
	libradio_set_state(&radio, LIBRADIO_STATE_SLEEP);
	while (1) {
		/*
		 * First off, halt the CPU until we've seen a clock tick.
		 */
		while (main_thread < 20)
			_sleep();
		main_thread = 0;
		putchar('.');
		libradio_loop(&radio);
		/*
		 * Look for the command sequence ^E\ to enter bootstrap mode.
		 */
		if (!sio_iqueue_empty()) {
			if ((ch = getchar()) == '\005')
				bs_state = 1;
			else {
				if (bs_state == 1 && ch == '\\')
					_bootstrap();
				else
					putchar('?');
				bs_state = 0;
			}
		}
	}
}

/*
 * As we don't have any custom command functions, this function just prints
 * whatever custom command was received. Useful for debugging, I suppose...
 */
void
operate(struct libradio *rp, struct packet *pp)
{
	printf("Oil Tank OPERATE function.\n");
	printf("Command: %d, len: %d\n", pp->cmd, pp->len);
}

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
	radio.ms_ticks += 100;
	if (radio.ms_ticks >= 60000) {
		radio.ms_ticks = 0;
		if (radio.tens_of_minutes < 145)
			radio.tens_of_minutes++;
	}
	libradio_heartbeat();
	main_thread++;
}
