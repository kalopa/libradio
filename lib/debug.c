/*
 * Copyright (c) 2020-23, Kalopa Robotics Limited.  All rights reserved.
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
 * Radio debug function.
 */
void
libradio_debug()
{
	printf("%d>", irq_fired);
	switch (getchar()) {
	case 'r':
		_bootstrap();
		break;
	case 'd':
		libradio_dump();
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
	case '0':
		libradio_get_part_info();
		printf("PART: %d/%d/%d/%d/%d/%d\n", radio.chip_rev, radio.part_id,
						radio.pbuild, radio.device_id,
						radio.customer, radio.rom_id);
		break;
	case '1':
		libradio_get_func_info();
		printf("FUNC: %d/%d/%d/%d/%d\n", radio.rev_ext, radio.rev_branch,
							radio.rev_int, radio.patch,
							radio.func);
		break;
	case '2':
		libradio_get_chip_status();
		printf("CHIP:%x/%x/%x\n", radio.chip_pending, radio.chip_status,
						radio.cmd_error);
		break;
	case '3':
		libradio_request_device_status();
		printf("DEVST:%d,CH%d\n", radio.curr_state, radio.curr_channel);
		break;
	case '4':
		libradio_get_int_status();
		printf("INTST: I%x/%x,P%x/%x,M%x/%x,C%x/%x\n",
					radio.int_pending, radio.int_status,
					radio.ph_pending, radio.ph_status,
					radio.modem_pending, radio.modem_status,
					radio.chip_pending, radio.chip_status);
		break;
	case '5':
		libradio_get_fifo_info(0);
		printf("FIFO RX:%d,TX:%d\n", radio.rx_fifo, radio.tx_fifo);
		break;
	case '6':
		libradio_ircal();
		break;
	case 'p':
		libradio_get_property(0x100, 4);
		break;
	default:
		putchar('\n');
	}
}

/*
 * Dump the radio output state
 */
void
libradio_dump()
{
	printf(">> LibRadio Overall State/Time:-\n");
	printf("State:\t%d\n", radio.state);
#if 0
	printf("Tens of Minutes:\t%d\n", radio.tens_of_minutes);
	printf("Millisecond Ticks:\t%d\n", radio.ms_ticks);
	printf("Slow Period:\t%d\n", radio.slow_period);
	printf("Fast Period:\t%d\n", radio.fast_period);
	printf("Period:\t%d\n", radio.period);
	printf("Heart Beat:\t%d\n", radio.heart_beat);
	printf("Date:\t%d\n", radio.date);
	printf("Main Ticks:\t%d\n", radio.main_ticks);
	printf("Timeout:\t%d\n", radio.timeout);
	printf("Catch IRQ:\t%d\n", radio.catch_irq);
	printf(">> Node and channel identification:-\n");
	printf("My Channel:\t%d\n", radio.my_channel);
	printf("Current Channel:\t%d\n", radio.curr_channel);
	printf("My Node ID:\t%d\n", radio.my_node_id);
	printf("Category:\t%d/%d\n", radio.cat1, radio.cat2);
	printf("Number:\t%d/%d\n", radio.num1, radio.num2);
	printf(">> Radio settings:-\n");
	printf("Chip Revision:\t%d\n", radio.chip_rev);
	printf("Part ID:\t%d\n", radio.part_id);
	printf("PBuild:\t%d\n", radio.pbuild);
	printf("Device ID:\t%d\n", radio.device_id);
	printf("Customer:\t%d\n", radio.customer);
	printf("ROM ID:\t%d\n", radio.rom_id);
	printf("Revision Ext:\t%d\n", radio.rev_ext);
	printf("Revision Branch:\t%d\n", radio.rev_branch);
	printf("Revision Int:\t%d\n", radio.rev_int);
	printf("Patch:\t%d\n", radio.patch);
	printf("Func:\t%d\n", radio.func);
	printf(">> Radio parameters:-\n");
	printf("# of RX Packets:\t%d\n", radio.npacket_rx);
	printf("# of TX Packets:\t%d\n", radio.npacket_tx);
	printf("Current State:\t%d\n", radio.curr_state);
	printf("Radio Active:\t%d\n", radio.radio_active);
	printf("RX FIFO:\t%d\n", radio.rx_fifo);
	printf("TX FIFO:\t%d\n", radio.tx_fifo);
	printf("INT Pending:\t%d\n", radio.int_pending);
	printf("INT Status:\t%d\n", radio.int_status);
	printf("PH Pending:\t%d\n", radio.ph_pending);
	printf("PH Status:\t%d\n", radio.ph_status);
	printf("Modem Pending:\t%d\n", radio.modem_pending);
	printf("Modem Status:\t%d\n", radio.modem_status);
	printf("Chip Pending:\t%d\n", radio.chip_pending);
	printf("Chip Status:\t%d\n", radio.chip_status);
	printf("Command Error:\t%d\n", radio.cmd_error);
	printf("Saw RX Packet:\t%d\n", radio.saw_rx);
#endif
}
