/*
 * Copyright (c) 2020-24, Kalopa Robotics Limited.  All rights reserved.
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
#define MAX_FIFO_SIZE		48
#define MAX_PACKET_SIZE		16
#define PACKET_HEADER_LEN	6
#define MAX_PAYLOAD_SIZE	(MAX_PACKET_SIZE - PACKET_HEADER_LEN)

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

#define RADIO_CMD_USER0			(RADIO_CMD_ADDITIONAL_BASE + 0)
#define RADIO_CMD_USER1			(RADIO_CMD_ADDITIONAL_BASE + 1)
#define RADIO_CMD_USER2			(RADIO_CMD_ADDITIONAL_BASE + 2)
#define RADIO_CMD_USER3			(RADIO_CMD_ADDITIONAL_BASE + 3)
#define RADIO_CMD_USER4			(RADIO_CMD_ADDITIONAL_BASE + 4)
#define RADIO_CMD_USER5			(RADIO_CMD_ADDITIONAL_BASE + 5)
#define RADIO_CMD_USER6			(RADIO_CMD_ADDITIONAL_BASE + 6)
#define RADIO_CMD_USER7			(RADIO_CMD_ADDITIONAL_BASE + 7)
#define RADIO_CMD_USER8			(RADIO_CMD_ADDITIONAL_BASE + 8)
#define RADIO_CMD_USER9			(RADIO_CMD_ADDITIONAL_BASE + 9)
#define RADIO_CMD_USER10		(RADIO_CMD_ADDITIONAL_BASE + 10)
#define RADIO_CMD_USER11		(RADIO_CMD_ADDITIONAL_BASE + 11)
#define RADIO_CMD_USER12		(RADIO_CMD_ADDITIONAL_BASE + 12)
#define RADIO_CMD_USER13		(RADIO_CMD_ADDITIONAL_BASE + 13)
#define RADIO_CMD_USER14		(RADIO_CMD_ADDITIONAL_BASE + 14)
#define RADIO_CMD_USER15		(RADIO_CMD_ADDITIONAL_BASE + 15)

#define RADIO_STATUS_STATIC			0
#define RADIO_STATUS_DYNAMIC		1
#define RADIO_STATUS_USER0			2
#define RADIO_STATUS_USER1			3
#define RADIO_STATUS_USER2			4
#define RADIO_STATUS_USER3			5

#define RADIO_CTLERR_INVALID_CHANNEL	1
#define RADIO_CTLERR_BUSY				2
#define RADIO_CTLERR_TOO_MUCH_DATA		3
#define RADIO_CTLERR_TOO_BIG			4
#define RADIO_CTLERR_NEWLINE			5
#define RADIO_CTLERR_NOT_ACTIVE			6
#define RADIO_CTLERR_POWER_FAIL			7
#define RADIO_CTLERR_BAD_CMD			8

/*
 * Packet to be transmitted. Multiple packets are folded up into one
 * fifo load (see the channel struct) and sent over the wire. Note
 * that the time stamp is encoded in front of the packet header and
 * not part of this struct. On receive, the time stamp is stripped
 * off first.
 *
 * We allow for a maximum of 10 bytes of actual payload data. This is
 * because the maximum packet size is 16 bytes, there are three "magic"
 * bytes sent during every transmission (myticks & cksum), and there
 * are three header bytes in the packet.
 */
typedef unsigned short ushort;
struct packet	{
	ushort	ticks;		/* Time stamp for the packet */
	uchar_t		node;		/* ID for receiver (0 is a broadcast) */
	uchar_t		cmd;		/* Command for the receiver */
	uchar_t		len;		/* Length of the data payload */
	uchar_t		csum;		/* Checksum for the overall packet */
	uchar_t		data[MAX_PAYLOAD_SIZE];
};

/*
 * Normal receivers just have a single channel entry, but the transmitter can
 * have multiple. One for every transmitting frequency in use.
 */
struct channel	{
	uchar_t		state;
	uchar_t		priority;
	struct packet	packet;
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
#define LIBRADIO_CHSTATE_TXRESPOND		5
#define LIBRADIO_CHSTATE_RXRESPONSE1	6
#define LIBRADIO_CHSTATE_RXRESPONSE2	7
#define LIBRADIO_CHSTATE_RXRESPONSE3	8
#define LIBRADIO_CHSTATE_RXRESPONSE4	9

#define LIBRADIO_WAIT_RXINT				01
#define LIBRADIO_WAIT_SERIAL			02
#define LIBRADIO_WAIT_TIMER				04

extern	volatile uchar_t	main_thread;

/*
 * Prototypes...
 */
void	libradio_init(uchar_t, uchar_t, uchar_t, uchar_t);
void	libradio_irq_enable(uchar_t);
uchar_t	libradio_get_state();
uchar_t	libradio_get_my_channel();
void	libradio_set_state(uchar_t);
void	libradio_set_clock(uchar_t, uchar_t);
uint_t	libradio_get_delay();
void	libradio_set_delay(uint_t);
int		libradio_tick_wait();
int		libradio_elapsed_second();
uchar_t	libradio_recv_start();
void	libradio_rxloop();
void	libradio_command(struct packet *);
uchar_t	libradio_wait();

uchar_t	libradio_recv(struct channel *, uchar_t);
uchar_t	libradio_send(struct channel *, uchar_t);
int		libradio_power_up();
void	libradio_power_down();
uint_t	libradio_get_ticks();
uchar_t	libradio_get_tom();

void	libradio_set_rx(uchar_t);
uchar_t	libradio_check_rx();
uchar_t	libradio_check_tx();
int		libradio_request_device_status();
void	libradio_get_property(uint_t, uchar_t);
void	libradio_set_property();
void	libradio_get_part_info();
void	libradio_get_func_info();
int		libradio_get_packet_info();
void	libradio_ircal();
void	libradio_protocol_cfg();
void	libradio_get_ph_status();
void	libradio_get_modem_status();
void	libradio_get_chip_status();
void	libradio_change_radio_state(uchar_t);
uchar_t	libradio_get_fifo_info(uchar_t);
void	libradio_get_int_status();
void	libradio_handle_packet();
uchar_t	libradio_irq_fired();
void	libradio_debug();

/*
 * Callback functions. operate() is called when a packet is received which is
 * not a built-in type. clock_speed() is used to slow down the clock during
 * low power modes.
 */
void	operate(struct packet *);
int		fetch_status(uchar_t, uchar_t [], int);
void	libradio_power_mode(uchar_t);
