/*
 * Copyright (c) 2020-21, Kalopa Robotics Limited.  All rights reserved.
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
 * A collection of utility functions for interfacing with the Si4463
 * radio module. Some of these could probably be split into separate
 * files so they're not pulled in by the linker unless actually needed.
 */
#include <stdio.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

#include <libavr.h>

#include "libradio.h"
#include "internal.h"

/*
 *
 */
int
libradio_request_device_status()
{
	spi_data[0] = SI4463_REQUEST_DEVICE_STATE;
	if (spi_send(1, 2) == 0)
		return(-1);
	radio.curr_state = spi_data[0];
	radio.curr_channel = spi_data[1];
	return(spi_data[0]);
}

/*
 *
 */
void
libradio_get_property(uint_t start_prop, uchar_t nprops)
{
	int i;

	printf(">>> Get property: %x (%d)\n", start_prop, nprops);
	spi_data[0] = SI4463_GET_PROPERTY;
	spi_data[1] = start_prop >> 8;
	spi_data[2] = nprops;
	spi_data[3] = start_prop & 0xff;
	if (spi_send(4, nprops + 2) == 0)
		return;
	for (i = 0; i < nprops + 2; i++)
		printf("PROP-%04x = %x\n", start_prop + i, spi_data[i]);
}

/*
 *
 */
void
libradio_set_property()
{
}

/*
 *
 */
void
libradio_get_part_info()
{
	printf(">>> Get Part Info...\n");
	spi_data[0] = SI4463_PART_INFO;
	if (spi_send(1, 8) == 0)
		return;
	printf("Chip Rev: %x\n", spi_data[0]);
	printf("Part: %x\n", (spi_data[1] << 8) | spi_data[2]);
	printf("PBuild: %x\n", spi_data[3]);
	printf("ID: %d\n", spi_data[4] << 8 | spi_data[5]);
	printf("Customer: %x\n", spi_data[6]);
	printf("ROM ID: %d\n", spi_data[7]);
}

/*
 *
 */
void
libradio_get_func_info()
{
	printf(">> Get func info...\n");
	spi_data[0] = SI4463_FUNC_INFO;
	if (spi_send(1, 6) == 0)
		return;
	printf("RevExt: %x\n", spi_data[0]);
	printf("RevBranch: %x\n", spi_data[1]);
	printf("RevInt: %x\n", spi_data[2]);
	printf("Patch: %x\n", spi_data[3] << 8 | spi_data[4]);
	printf("Func: %x\n", spi_data[5]);
}

/*
 *
 */
void
libradio_get_packet_info()
{
}

/*
 *
 */
void
libradio_ircal()
{
}

/*
 *
 */
void
libradio_protocol_cfg()
{
}

/*
 *
 */
void
libradio_get_ph_status()
{
	spi_data[0] = SI4463_GET_PH_STATUS;
	spi_data[1] = 0xff;
	if (spi_send(2, 2) == 0)
		return;
	radio.ph_pending = spi_data[0];
	radio.ph_status = spi_data[1];
}

/*
 *
 */
void
libradio_get_modem_status()
{
	spi_data[0] = SI4463_GET_MODEM_STATUS;
	if (spi_send(1, 6) == 0)
		return;
	radio.modem_pending = spi_data[0];
	radio.modem_status = spi_data[1];
}

/*
 *
 */
void
libradio_get_chip_status()
{
	spi_data[0] = SI4463_GET_CHIP_STATUS;
	spi_data[1] = 0x7f;
	if (spi_send(2, 3) == 0)
		return;
	radio.chip_pending = spi_data[0];
	radio.chip_status = spi_data[1];
	radio.cmd_error = spi_data[2];
}

/*
 *
 */
void
libradio_change_radio_state(uchar_t new_state)
{
	printf(">>> Change state to %d...\n", new_state);
	spi_data[0] = SI4463_CHANGE_STATE;
	spi_data[1] = new_state & 0xf;
	spi_send(2, 0);
}

/*
 *
 */
void
libradio_get_fifo_info(uchar_t clrf)
{
	spi_data[0] = SI4463_FIFO_INFO;
	spi_data[1] = clrf;
	if (spi_send(2, 2) == 0)
		return;
	radio.rx_fifo = spi_data[0];
	radio.tx_fifo = spi_data[1];
}

/*
 *
 */
void
libradio_get_int_status()
{
	spi_data[0] = SI4463_GET_INT_STATUS;
	spi_data[1] = 0;
	spi_data[2] = 0;
	spi_data[3] = 0;
	if (spi_send(4, 8) == 0)
		return;
	radio.int_pending = spi_data[0];
	radio.int_status = spi_data[1];
	radio.ph_pending = spi_data[2];
	radio.ph_status = spi_data[3];
	radio.modem_pending = spi_data[4];
	radio.modem_status = spi_data[5];
	radio.chip_pending = spi_data[6];
	radio.chip_status = spi_data[7];
}
