/*
 * Copyright (c) 2020-21, Kalopa Robotics Limited.  All rights
 * reserved.
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
 */

#define MAX_RADIO_CHANNELS	6
#define MAX_PAYLOAD			46

/*
 * REBOOT state is set after a reboot (obviously) and will remain there
 * until a radio message is received. If no radio message is received
 * within five minutes then the system should go to a SHUTDOWN state.
 * The LOWBATT state means the battery is below threshold and we will
 * sleep until it reaches a usable voltage. SLEEP means we are ready
 * for operational work, but none is given. The system is off-air,
 * quiet and minimising power consumption. It will take a wakeup message
 * to activate the device again. The SHUTDOWN state is a transient state
 * to allow the device to do an orderly shutdown. From SHUTDOWN, the
 * device enters the SLEEP state. ACTIVE means what it says. The system
 * has a radio channel ID and is communicating/active. All systems
 * are GO! An ERROR state needs a reset radio signal to clear back to
 * ACTIVE. Also, if in an error state for five minutes, the device will
 * go to SHUTDOWN (and on to SLEEP).
 */
#define LIBRADIO_STATE_REBOOT		0
#define LIBRADIO_STATE_LOWBATT		1
#define LIBRADIO_STATE_SLEEP		2
#define LIBRADIO_STATE_SHUTDOWN		3
#define LIBRADIO_STATE_ERROR		4
#define LIBRADIO_STATE_ACTIVE		5
#define NLIBRADIO_STATES			6

#define RADIO_CMD_FIRMWARE			0
#define RADIO_CMD_STATUS			1
#define RADIO_CMD_ACTIVATE			2
#define RADIO_CMD_DEACTIVATE		3
#define RADIO_CMD_SET_TIME			4
#define RADIO_CMD_SET_DATE			5
#define RADIO_CMD_READ_EEPROM		6
#define RADIO_CMD_WRITE_EEPROM		7

#define RADIO_CMD_ADDITIONAL_BASE	16

/*
 * Main status. Here is where the various status parameters exchanged with
 * the Si4463 radio are saved.
 */
struct libradio {
	/*
	 * Overall state
	 */
	uchar_t		state;
	uchar_t		tens_of_minutes;
	uint_t		ms_ticks;
	uint_t		date;
	uchar_t		my_channel;
	uchar_t		my_node_id;
	uchar_t		unique_code1;
	uchar_t		unique_code2;
	/*
	 * Radio parameters.
	 */
	uint_t		npacket_rx;
	uint_t		npacket_tx;
	uchar_t		curr_channel;
	uchar_t		curr_state;
	uchar_t		radio_active;
	uchar_t		rx_fifo;
	uchar_t		tx_fifo;
	uchar_t		int_pending;
	uchar_t		int_status;
	uchar_t		ph_pending;
	uchar_t		ph_status;
	uchar_t		modem_pending;
	uchar_t		modem_status;
	uchar_t		chip_pending;
	uchar_t		chip_status;
	uchar_t		cmd_error;
};

/*
 * Packet to be transmitted. Multiple packets are folded up into one
 * payload (see the channel struct) and sent over the wire.
 */
struct packet	{
	uchar_t		node;
	uchar_t		len;
	uchar_t		cmd;
	uchar_t		data[1];
};

struct channel	{
	uchar_t		offset;
	uchar_t		state;
	uchar_t 	payload[MAX_PAYLOAD];
};

#define CHANNEL_STATE_EMPTY			0
#define CHANNEL_STATE_ADDING		1
#define CHANNEL_STATE_TRANSMIT		2

/*
 * Prototypes...
 */
void	libradio_init(struct libradio *);
void	libradio_set_state(struct libradio *, uchar_t);
void	libradio_loop(struct libradio *);

uchar_t	libradio_recv(struct libradio *, struct channel *, uchar_t);
uchar_t	libradio_send(struct libradio *, struct channel *, uchar_t);
int		libradio_power_up(struct libradio *);
int		libradio_request_device_status(struct libradio *);
void	libradio_get_property(struct libradio *, uint_t, uchar_t);
void	libradio_set_property(struct libradio *);
void	libradio_get_part_info(struct libradio *);
void	libradio_get_func_info(struct libradio *);
void	libradio_get_packet_info(struct libradio *);
void	libradio_ircal(struct libradio *);
void	libradio_protocol_cfg(struct libradio *);
void	libradio_get_ph_status(struct libradio *);
void	libradio_get_modem_status(struct libradio *);
void	libradio_get_chip_status(struct libradio *);
void	libradio_change_radio_state(struct libradio *, uchar_t);
void	libradio_get_fifo_info(struct libradio *, uchar_t);
void	libradio_get_int_status(struct libradio *);

void	libradio_heartbeat();
void	libradio_set_song(uchar_t);

void	operate(struct libradio *, struct packet *);
