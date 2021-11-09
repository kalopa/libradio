/*
 * Copyright (c) 2020-21, Kalopa Robotics Limited.  All rights reserved.
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
 * This is the main include file for the library. It has the definitions
 * for the states, the valid (built-in) commands, and the packet &
 * channel structure definitions. It also includes prototypes for the
 * remaining functions.
 */

#define MAX_RADIO_CHANNELS	6
#define MAX_FIFO_SIZE		46
#define MAX_PACKET_SIZE		16

/*
 * State machine used to manager radio operations. The states are
 * extensible. Just make sure the additional states start with
 * NLIBRADIO_STATES and go up from there. The states are documented in the
 * README file.
 */
#define LIBRADIO_STATE_STARTUP		0
#define LIBRADIO_STATE_ERROR		1
#define LIBRADIO_STATE_COLD			2
#define LIBRADIO_STATE_WARM			3
#define LIBRADIO_STATE_LISTEN		4
#define LIBRADIO_STATE_ACTIVE		5
#define NLIBRADIO_STATES			6

#define RADIO_CMD_NOOP				0
#define RADIO_CMD_FIRMWARE			1
#define RADIO_CMD_STATUS			2
#define RADIO_CMD_ACTIVATE			3
#define RADIO_CMD_DEACTIVATE		4
#define RADIO_CMD_SET_TIME			5
#define RADIO_CMD_SET_DATE			6
#define RADIO_CMD_READ_EEPROM		7
#define RADIO_CMD_WRITE_EEPROM		8

#define RADIO_CMD_ADDITIONAL_BASE	16

/*
 * Packet to be transmitted. Multiple packets are folded up into one
 * fifo load (see the channel struct) and sent over the wire. Note
 * that the time stamp is encoded in front of the packet header and
 * not part of this struct. On receive, the time stamp is stripped
 * off first.
 */
struct packet	{
	uchar_t		node;		/* ID for receiver (0 is a broadcast) */
	uchar_t		len;		/* Length of the data payload */
	uchar_t		cmd;		/* Command for the receiver */
	uchar_t		csum;		/* 8-bit checksum */
	uchar_t		data[1];	/* Zero or more bytes of command data */
};

/*
 * Normal receivers just have a single channel entry, but the transmitter can
 * have multiple. One for every transmitting frequency in use.
 */
struct channel	{
	uchar_t		offset;
	uchar_t		state;
	uchar_t 	payload[MAX_FIFO_SIZE];
};

#define LIBRADIO_CHSTATE_EMPTY			0
#define LIBRADIO_CHSTATE_ADDING			1
#define LIBRADIO_CHSTATE_TRANSMIT		2

extern	volatile uchar_t	main_thread;

/*
 * Prototypes...
 */
void	libradio_init(uchar_t, uchar_t, uchar_t, uchar_t);
void	libradio_set_state(uchar_t);
void	libradio_set_clock(uchar_t, uchar_t);
void	libradio_loop();

uchar_t	libradio_recv(struct channel *, uchar_t);
uchar_t	libradio_send(struct channel *, uchar_t);
int		libradio_power_up();
int		libradio_request_device_status();
void	libradio_get_property(uint_t, uchar_t);
void	libradio_set_property();
void	libradio_get_part_info();
void	libradio_get_func_info();
void	libradio_get_packet_info();
void	libradio_ircal();
void	libradio_protocol_cfg();
void	libradio_get_ph_status();
void	libradio_get_modem_status();
void	libradio_get_chip_status();
void	libradio_change_radio_state(uchar_t);
void	libradio_get_fifo_info(uchar_t);
void	libradio_get_int_status();

/*
 * Callback functions. operate() is called when a packet is received which is
 * not a built-in type. clock_speed() is used to slow down the clock during
 * low power modes.
 */
void	operate(struct packet *);
void	power_mode(uchar_t);
