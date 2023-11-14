/*
 * Copyright (c) 2020-23, Kalopa Robotics Limited.  All rights reserved.
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
#define LIBRADIO_STATE_LOW_BATTERY	2
#define LIBRADIO_STATE_COLD			3
#define LIBRADIO_STATE_WARM			4
#define LIBRADIO_STATE_LISTEN		5
#define LIBRADIO_STATE_ACTIVE		6
#define NLIBRADIO_STATES			7

#define RADIO_CMD_NOOP				0
#define RADIO_CMD_FIRMWARE			1
#define RADIO_CMD_STATUS			2
#define RADIO_CMD_ACTIVATE			3
#define RADIO_CMD_DEACTIVATE		4
#define RADIO_CMD_SET_TIME			5
#define RADIO_CMD_SET_DATE			6
#define RADIO_CMD_READ_EEPROM		7
#define RADIO_CMD_WRITE_EEPROM		8
#define RADIO_STATUS_RESPONSE		9
#define RADIO_EEPROM_RESPONSE		10

#define RADIO_CMD_ADDITIONAL_BASE	16

#define RADIO_STATUS_DYNAMIC		0
#define RADIO_STATUS_STATIC			1

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
	uchar_t		data[1];	/* Zero or more bytes of command data */
};

#define PACKET_HEADER_SIZE		4

/*
 * Normal receivers just have a single channel entry, but the transmitter can
 * have multiple. One for every transmitting frequency in use.
 */
struct channel	{
	uchar_t		offset;
	uchar_t		state;
	uchar_t		priority;
	uchar_t 	payload[MAX_FIFO_SIZE];
};

/*
 * Channel states. DISABLED is obvious. READ means the channel is only used
 * by the transmit code as an RX channel. EMPTY means it doesn't have any
 * data. ADDING means that some code is currently receiving a packet for
 * transmission and TRANSMIT means it has at least one packet ready for TX.
 */
#define LIBRADIO_CHSTATE_DISABLED		0
#define LIBRADIO_CHSTATE_READ			1
#define LIBRADIO_CHSTATE_EMPTY			2
#define LIBRADIO_CHSTATE_ADDING			3
#define LIBRADIO_CHSTATE_TRANSMIT		4

extern	volatile uchar_t	main_thread;

/*
 * Prototypes...
 */
void	libradio_init(uchar_t, uchar_t, uchar_t, uchar_t);
void	libradio_irq_enable(uchar_t);
uchar_t	libradio_get_state();
void	libradio_set_state(uchar_t);
void	libradio_set_clock(uchar_t, uchar_t);
void	libradio_set_delay(uint_t);
int		libradio_tick_wait();
int		libradio_elapsed_second();
void	libradio_rxloop();
void	libradio_command(struct packet *);

uchar_t	libradio_recv(struct channel *, uchar_t);
uchar_t	libradio_send(struct channel *, uchar_t);
int		libradio_power_up();
void	libradio_power_down();
uint_t	libradio_get_ticks();
uchar_t	libradio_get_tom();

void	libradio_set_rx(uchar_t);
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
void	libradio_handle_packet();
void	libradio_debug();
void	libradio_dump();

/*
 * Callback functions. operate() is called when a packet is received which is
 * not a built-in type. clock_speed() is used to slow down the clock during
 * low power modes.
 */
void	operate(struct packet *);
int		fetch_status(uchar_t, uchar_t [], int);
void	power_mode(uchar_t);
