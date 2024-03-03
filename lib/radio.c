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
 * Request the status of the radio from the radio itself. It returns the
 * device status, such as SI4463_STATE_READY. It also implicitly sets
 * radio.curr_channel which is the current channel/frequency we are on.
 */
int
libradio_request_device_status()
{
	int i;

	spi_data[0] = SI4463_REQUEST_DEVICE_STATE;
	if ((i = spi_send(1, 2)) != SPI_SEND_OK) {
		spi_error(i);
		return(-1);
	}
	radio.curr_channel = spi_data[1];
	return(radio.curr_state = spi_data[0]);
}

/*
 * Retrieve a property from the radio. The properties are specified
 * as two-byte parameters. See AN625.pdf from Silicon Labs for more
 * information on the properties.
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
	if ((i = spi_send(4, nprops + 2)) != SPI_SEND_OK) {
		spi_error(i);
		return;
	}
	for (i = 0; i < nprops + 2; i++)
		printf("PROP-%04x = %x\n", start_prop + i, spi_data[i]);
}

/*
 * Set a radio property. The properties are specified as two-byte
 * parameters. See AN625.pdf from Silicon Labs for more information on
 * the properties. Currently unused and unimplemented.
 */
void
libradio_set_property()
{
}

/*
 * Return the radio "part" information. This retrieves the chip revision,
 * the part ID, as well as the build, device ID and ROM ID. See the chip
 * documentation for more information.
 */
void
libradio_get_part_info()
{
	spi_data[0] = SI4463_PART_INFO;
	if (spi_send(1, 8) != SPI_SEND_OK)
		return;
	radio.chip_rev = spi_data[0];
	radio.part_id = (spi_data[1] << 8) | spi_data[2];
	radio.pbuild = spi_data[3];
	radio.device_id = (spi_data[4] << 8) | spi_data[5];
	radio.customer = spi_data[6];
	radio.rom_id = spi_data[7];
}

/*
 * Return the radio "functional" information. This retrieves the revision
 * numbering and the radio function. See the chip documentation for more
 * information.
 */
void
libradio_get_func_info()
{
	spi_data[0] = SI4463_FUNC_INFO;
	if (spi_send(1, 6) != SPI_SEND_OK)
		return;
	radio.rev_ext = spi_data[0];
	radio.rev_branch = spi_data[1];
	radio.rev_int = spi_data[2];
	radio.patch = (spi_data[3] << 8) | spi_data[4];
	radio.func = spi_data[5];
}

/*
 * Return the radio packet information. See the chip documentation for
 * more information.
 */
int
libradio_get_packet_info()
{
	spi_data[0] = SI4463_PACKET_INFO;
	if (spi_send(1, 2) != SPI_SEND_OK)
		return(-1);
	return((spi_data[0] << 8) | spi_data[1]);
}

/*
 * Calibrate the receiver image rejection. See the chip documentation
 * for more information. Currently unimplemented and unused.
 */
void
libradio_ircal()
{
}

/*
 * Set up the radio chip for a specific packet protocol. See the chip
 * documentation for more information. Currently unimplemented.
 */
void
libradio_protocol_cfg()
{
}

/*
 * Retrieve the Packet Handler status from the radio. See the chip
 * documentation for details of the status returned.
 */
void
libradio_get_ph_status()
{
	spi_data[0] = SI4463_GET_PH_STATUS;
	spi_data[1] = 0xff;
	if (spi_send(2, 2) != SPI_SEND_OK)
		return;
	radio.ph_pending = spi_data[0];
	radio.ph_status = spi_data[1];
}

/*
 * Retrieve the Modem status from the radio. See the chip documentation
 * for details of the status returned.
 */
void
libradio_get_modem_status()
{
	spi_data[0] = SI4463_GET_MODEM_STATUS;
	if (spi_send(1, 6) != SPI_SEND_OK)
		return;
	radio.modem_pending = spi_data[0];
	radio.modem_status = spi_data[1];
	radio.current_rssi = spi_data[2];
	radio.latch_rssi = spi_data[3];
	radio.ant1_rssi = spi_data[4];
	radio.ant2_rssi = spi_data[5];
}

/*
 * Retrieve the overall chip status from the radio. See the chip
 * documentation for details of the status returned.
 */
void
libradio_get_chip_status()
{
	spi_data[0] = SI4463_GET_CHIP_STATUS;
	spi_data[1] = 0x7f;
	if (spi_send(2, 3) != SPI_SEND_OK)
		return;
	radio.chip_pending = spi_data[0];
	radio.chip_status = spi_data[1];
	radio.cmd_error = spi_data[2];
}

/*
 * Change the actual radio state. Usually used to switch from
 * SI4463_STATE_RX to SI4463_STATE_TX states.
 */
void
libradio_change_radio_state(uchar_t new_state)
{
	spi_data[0] = SI4463_CHANGE_STATE;
	spi_data[1] = new_state & 0xf;
	spi_send(2, 0);
	radio.curr_state = new_state;
}

/*
 * Get the current FIFO status. Includes the number of bytes in the RX and
 * TX. The clrf argument is also used to clear the count. 01 clears the
 * TX fifo and 02 clears the RX fifo. Returns the RX FIFO count.
 */
uchar_t
libradio_get_fifo_info(uchar_t clrf)
{
	spi_data[0] = SI4463_FIFO_INFO;
	spi_data[1] = clrf;
	if (spi_send(2, 2) != SPI_SEND_OK)
		return(0);
	radio.tx_fifo = spi_data[1];
	return(radio.rx_fifo = spi_data[0]);
}

/*
 * Get the radio device interrupt status, clearing all the bits in
 * the process. Sets all eight of the int/ph/modem/chip and pending/status
 * values in the radio struct.
 */
void
libradio_get_int_status()
{
	spi_data[0] = SI4463_GET_INT_STATUS;
	spi_data[1] = spi_data[2] = spi_data[3] = 0;
	if (spi_send(4, 8) != SPI_SEND_OK)
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
