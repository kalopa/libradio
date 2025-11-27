/*
 * Copyright (c) 2020-24, Kalopa Robotics Limited.  All rights reserved.
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

struct libradio		radio;

/*
 * Our initial NodeID is zero, until we are activated. Also, while we are
 * waiting for activation, listen to the unactivated group. This node is
 * identified by four bytes. C1/C2 and N1/N2. Generally the first two
 * represent the class of device and the last two the device instance.
 * C1/C2 would be hard-coded in the caller, while N1/N2 would be pulled
 * from EEPROM. The node is activated by identifying the unique four byte
 * tuple. Don't track elapsed time until we have real values for the date.
 */
void
libradio_init(uchar_t c1, uchar_t c2, uchar_t n1, uchar_t n2)
{
	memset((void *)&radio, 0, sizeof(struct libradio));
	libradio_set_state(LIBRADIO_STATE_STARTUP);
	libradio_set_clock(1, 16);
	radio.period = radio.fast_period;
	radio.tens_of_minutes = 0xff;
	radio.main_ticks = radio.tick_count = 5;
	radio.curr_state = SI4463_STATE_SLEEP;
	radio.cat1 = c1;
	radio.cat2 = c2;
	radio.num1 = n1;
	radio.num2 = n2;
	pkt_init();
	libradio_get_fifo_info(03);
}

/*
 * Set the fast and slow clock parameters. Each argument defines the
 * number of 10 millisecond ticks per clock interrupt in each fast or
 * slow mode. The default is a 10ms clock (fast == 1) in fast mode and
 * a 160ms clock in slow mode (slow == 16).
 *
 * For reference, 'ms_ticks += period' on each clock interrupt.
 */
void
libradio_set_clock(uchar_t fast, uchar_t slow)
{
	radio.fast_period = radio.period = fast;
	radio.slow_period = slow;
}
