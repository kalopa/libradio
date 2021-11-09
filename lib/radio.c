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

#if 0
/*
 * Received a character from the radio receiver.
 */
void
radio_rx(uchar_t ch)
{
	if (recvp == NULL) {
		/*
		 * We don't have anywhere to stick this data, so disable the
		 * receive interrupt, and return.
		 */
		/* FIXME: disable SPI RX Interrupts */
		return;
	}
	if (ch == 0xc0) {
		recvp->state = PKT_STATE_ADDR;
		escape = 0;
		return;
	}
	if (recvp->state == PKT_STATE_HEADER)
		return;
	if (ch == 0xc3) {
		escape = 1;
		return;
	}
	if (escape) {
		ch |= 0x80;
		escape = 0;
	}
	switch (recvp->state) {
	case PKT_STATE_ADDR:
		/*
		 * Received the Node ID
		 */
		if (ch == my_node_id || ch == my_group_id || ch == 0xff) {
			recvp->cksum = ch;
			recvp->state = PKT_STATE_CMD;
		} else
			recvp->state = PKT_STATE_HEADER;
		break;

	case PKT_STATE_CMD:
		/*
		 * Received the command
		 */
		recvp->cmd = ch;
		_cksum(recvp, ch);
		recvp->state = (ch & 3) > 0 ? PKT_STATE_DATA1 : PKT_STATE_CKSUM;
		break;

	case PKT_STATE_DATA1:
		/*
		 * Data byte #1
		 */
		recvp->data1 = ch;
		_cksum(recvp, ch);
		recvp->state = (ch & 3) > 1 ? PKT_STATE_DATA2 : PKT_STATE_CKSUM;
		break;

	case PKT_STATE_DATA2:
		/*
		 * Data byte #2
		 */
		recvp->data2 = ch;
		_cksum(recvp, ch);
		recvp->state = (ch & 3) > 2 ? PKT_STATE_DATA3 : PKT_STATE_CKSUM;
		break;

	case PKT_STATE_DATA3:
		/*
		 * Data byte #3
		 */
		recvp->data3 = ch;
		_cksum(recvp, ch);
		recvp->state = PKT_STATE_CKSUM;
		break;

	case PKT_STATE_CKSUM:
		/*
		 * Checksum
		 */
		if (recvp->cksum == ch)
			recvp->state = PKT_STATE_READY;
		else
			recvp->state = PKT_STATE_HEADER;
		break;
	}
}

/*
 * Transmit a character via the radio.
 */
void
radio_tx()
{
	if (escape != 0) {
		_send(escape & 0x7f, 1);
		return;
	}
	if (sendp == NULL || sendp->state == PKT_STATE_READY) {
		/*
		 * Nothing to send - disable interrupts and return.
		 */
		/* Disable SPI TX Interrupt */
		return;
	}
	switch (sendp->state) {
	case PKT_STATE_HEADER:
		/*
		 * Send a header byte.
		 */
		_send(0xc0, 1);
		sendp->state = PKT_STATE_ADDR;
		break;

	case PKT_STATE_ADDR:
		/*
		 * Send the address byte.
		 */
		_send(my_node_id, 0);
		sendp->state = PKT_STATE_CMD;
		sendp->cksum = my_node_id;
		break;

	case PKT_STATE_CMD:
		/*
		 * Send the command byte.
		 */
		_send(sendp->cmd, 0);
		_cksum(sendp, sendp->cmd);
		sendp->state = (sendp->cmd & 03) > 0 ? PKT_STATE_DATA1 : PKT_STATE_CKSUM;
		break;

	case PKT_STATE_DATA1:
		/*
		 * Send the first data byte.
		 */
		_send(sendp->data1, 0);
		_cksum(sendp, sendp->data1);
		sendp->state = (sendp->cmd & 03) > 1 ? PKT_STATE_DATA2 : PKT_STATE_CKSUM;
		break;

	case PKT_STATE_DATA2:
		/*
		 * Send the second data byte.
		 */
		_send(sendp->data2, 0);
		_cksum(sendp, sendp->data2);
		sendp->state = (sendp->cmd & 03) > 2 ? PKT_STATE_DATA3 : PKT_STATE_CKSUM;
		break;

	case PKT_STATE_DATA3:
		/*
		 * Send the third data byte.
		 */
		_send(sendp->data3, 0);
		_cksum(sendp, sendp->data3);
		sendp->state = PKT_STATE_CKSUM;
		break;

	case PKT_STATE_CKSUM:
		/*
		 * Send the checksum.
		 */
		_send(sendp->cksum, 0);
		sendp->state = PKT_STATE_READY;
		break;
	}
}

/*
 *
 */
void
_send(uchar_t ch, int force)
{
	if (force == 0) {
		if (ch >= 0xc0 && ch <= 0xc3) {
			escape = ch;
			ch = 0xc3;
		}
	} else
		escape = 0;
	/* Send CH via the SPI */
}

/*
 * Compute the checksum
 */
void
_cksum(struct pkt *pp, uchar_t ch)
{
	pp->cksum = (pp->cksum << 1) ^ ch;
}
#endif
