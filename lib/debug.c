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

/*
 * Radio debug function. Read a character from the serial port, and
 * perform some sort of radio function as a result. Useful for debugging
 * the state of the radio and/or the state of the communications layer.
 */
void
libradio_debug()
{
	char ch;

	if ((ch = getchar()) == '\n' || ch == '\r') {
		putchar('\n');
		return;
	}
	printf("%d>", libradio_irq_fired());
	switch (ch) {
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		ch -= '0';
		printf("-> %d\n", ch);
		radio.my_channel = ch;
		break;
	case 'r':
		_bootstrap();
		break;
	case 'i':
		libradio_irq_enable(1);
		printf("IRQ-ena\n");
		break;
	case 'o':
		libradio_irq_enable(0);
		printf("IRQ-dis\n");
		break;
	case 'h':
		libradio_handle_packet();
		printf("handle packet\n");
		break;
	case 'p':
		libradio_get_part_info();
		printf("PART: %d/%d/%d/%d/%d/%d\n", radio.chip_rev, radio.part_id,
						radio.pbuild, radio.device_id,
						radio.customer, radio.rom_id);
		break;
	case 'f':
		libradio_get_func_info();
		printf("FUNC: %d/%d/%d/%d/%d\n", radio.rev_ext, radio.rev_branch,
							radio.rev_int, radio.patch,
							radio.func);
		break;
	case 'c':
		libradio_get_chip_status();
		printf("CHIP:%x/%x/%x\n", radio.chip_pending, radio.chip_status,
						radio.cmd_error);
		break;
	case 'd':
		libradio_request_device_status();
		printf("DEVST:%d,CH%d\n", radio.curr_state, radio.curr_channel);
		break;
	case 's':
		libradio_get_int_status();
		printf("INTST:I%x/%x,P%x/%x,M%x/%x,C%x/%x\n",
					radio.int_pending, radio.int_status,
					radio.ph_pending, radio.ph_status,
					radio.modem_pending, radio.modem_status,
					radio.chip_pending, radio.chip_status);
		break;
	case 'G':
		libradio_get_fifo_info(03);
	case 'g':
		libradio_get_fifo_info(0);
		printf("FIFO RX:%d,TX:%d\n", radio.rx_fifo, radio.tx_fifo);
		break;
	case 'z':
		libradio_ircal();
		break;
	case 'P':
		libradio_get_property(0x100, 4);
		break;
	default:
		putchar('\n');
	}
}
