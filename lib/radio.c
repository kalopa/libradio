/*
 * Copyright (c) 2020-25, Kalopa Robotics Limited.  All rights reserved.
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
 * Chip-level status and property helpers for the Si4463 - none of this
 * is aware of our over-the-air packet format, just the chip's own
 * command set. See AN625.pdf from Silicon Labs for details.
 */
#include <stdio.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

#include <libavr.h>

#include "libradio.h"
#include "internal.h"

/*
 * Request the status of the radio from the radio itself. It returns the
 * device status, such as SI4463_STATE_READY.
 */
int
libradio_request_device_status()
{
	int i;

	pkt_data[0] = SI4463_REQUEST_DEVICE_STATE;
	if ((i = pkt_send(1, 2)) != SPI_SEND_OK) {
		pkt_error(i);
		return(-1);
	}
	return(radio.curr_state = pkt_data[0]);
}

/*
 * Return the radio "part" information. This retrieves the chip revision,
 * the part ID, as well as the build, device ID and ROM ID.
 */
void
libradio_get_part_info()
{
	pkt_data[0] = SI4463_PART_INFO;
	if (pkt_send(1, 8) != SPI_SEND_OK)
		return;
	radio.chip_rev = pkt_data[0];
	radio.part_id = (pkt_data[1] << 8) | pkt_data[2];
	radio.pbuild = pkt_data[3];
	radio.device_id = (pkt_data[4] << 8) | pkt_data[5];
	radio.customer = pkt_data[6];
	radio.rom_id = pkt_data[7];
}

/*
 * Return the radio "functional" information. This retrieves the revision
 * numbering and the radio function.
 */
void
libradio_get_func_info()
{
	pkt_data[0] = SI4463_FUNC_INFO;
	if (pkt_send(1, 6) != SPI_SEND_OK)
		return;
	radio.rev_ext = pkt_data[0];
	radio.rev_branch = pkt_data[1];
	radio.rev_int = pkt_data[2];
	radio.patch = (pkt_data[3] << 8) | pkt_data[4];
	radio.func = pkt_data[5];
}

/*
 * Calibrate the receiver image rejection. Currently unimplemented.
 */
void
libradio_ircal()
{
}

/*
 * Retrieve the overall chip status from the radio.
 */
void
libradio_get_chip_status()
{
	pkt_data[0] = SI4463_GET_CHIP_STATUS;
	pkt_data[1] = 0x7f;
	if (pkt_send(2, 3) != SPI_SEND_OK)
		return;
	radio.chip_pending = pkt_data[0];
	radio.chip_status = pkt_data[1];
	radio.cmd_error = pkt_data[2];
}

/*
 * Change the actual radio state. Usually used to switch from
 * SI4463_STATE_RX to SI4463_STATE_TX states.
 */
void
libradio_change_radio_state(uchar_t new_state)
{
	pkt_data[0] = SI4463_CHANGE_STATE;
	pkt_data[1] = new_state & 0xf;
	pkt_send(2, 0);
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
	pkt_data[0] = SI4463_FIFO_INFO;
	pkt_data[1] = clrf;
	if (pkt_send(2, 2) != SPI_SEND_OK)
		return(0);
	radio.tx_fifo = pkt_data[1];
	return(radio.rx_fifo = pkt_data[0]);
}

/*
 * Get the radio device interrupt status, clearing all the bits in
 * the process.
 */
void
libradio_get_int_status()
{
	pkt_data[0] = SI4463_GET_INT_STATUS;
	pkt_data[1] = pkt_data[2] = pkt_data[3] = 0;
	if (pkt_send(4, 8) != SPI_SEND_OK)
		return;
	radio.int_pending = pkt_data[0];
	radio.int_status = pkt_data[1];
	radio.ph_pending = pkt_data[2];
	radio.ph_status = pkt_data[3];
	radio.modem_pending = pkt_data[4];
	radio.modem_status = pkt_data[5];
	radio.chip_pending = pkt_data[6];
	radio.chip_status = pkt_data[7];
}
