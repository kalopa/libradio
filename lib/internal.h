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
 * This is the internal library for the libradio module. It isn't really
 * designed to be included by the main application. It should be able
 * to do what it needs, with the published functions. The internals of
 * the library are defined here.
 */
#define MAX_SPI_BLOCK				20

#define SI4463_PACKET_LEN			16

#define SI4463_NOP					0x00
#define SI4463_PART_INFO			0x01
#define SI4463_POWER_UP				0x02
#define SI4463_FUNC_INFO			0x10
#define SI4463_SET_PROPERTY			0x11
#define SI4463_GET_PROPERTY			0x12
#define SI4463_GPIO_PIN_CFG			0x13
#define SI4463_FIFO_INFO			0x15
#define SI4463_GET_INT_STATUS		0x20
#define SI4463_REQUEST_DEVICE_STATE	0x33
#define SI4463_CHANGE_STATE			0x34
#define SI4463_OFFLINE_RECAL		0x38
#define SI4463_READ_CMD_BUFF		0x44
#define SI4463_FRR_A_READ			0x50
#define SI4463_FRR_B_READ			0x51
#define SI4463_FRR_C_READ			0x53
#define SI4463_FRR_D_READ			0x57

#define SI4463_IRCAL				0x17
#define SI4463_IRCAL_MANUAL			0x19

#define SI4463_START_TX				0x31
#define SI4463_TX_HOP				0x37
#define SI4463_WRITE_TX_FIFO		0x66

#define SI4463_PACKET_INFO			0x16
#define SI4463_GET_MODEM_STATUS		0x22
#define SI4463_START_RX				0x32
#define SI4463_RX_HOP				0x36
#define SI4463_READ_RX_FIFO			0x77

#define SI4463_GET_ADC_READING		0x14
#define SI4463_GET_PH_STATUS		0x21
#define SI4463_GET_CHIP_STATUS		0x23

#define SI4463_STATE_NOCHANGE		0
#define SI4463_STATE_SLEEP			1
#define SI4463_STATE_SPI_ACTIVE		2
#define SI4463_STATE_READY			3
#define SI4463_STATE_READY2			4
#define SI4463_STATE_TX_TUNE		5
#define SI4463_STATE_RX_TUNE		6
#define SI4463_STATE_TX				7
#define SI4463_STATE_RX				8

/*
 * Return codes from pkt_send()
 */
#define SPI_SEND_OK				0
#define SPI_SEND_WRITE_FAIL		1
#define SPI_SEND_STATUS_FAIL	2
#define SPI_SEND_ACK_FAIL		3
#define SPI_SEND_READ_FAIL		4
#define SPI_SEND_TIMEOUT		5

/*
 * Main status. Here is where the various status parameters exchanged with
 * the Si4463 radio are saved.
 *
 * state - The current state of the system: LIBRADIO_STATE_{x}
 * tens_of_minutes - Time of day in 10 minute increments (255 = off)
 * ms_ticks - Count of 10ms ticks
 * slow_period - Clock period (in 10ms) running slowly (usually 16)
 * fast_period - Clock period (in 10ms) running normally (usually 1)
 * period - No. of 10ms ticks per interrupt (see slow/fast)
 * heart_beat - Timer used for the LED song
 * date - Current date (user-defined)
 * timeout - How long to go from WARM to COLD
 * all_ticks - No. of clock ticks since boot
 * main_ticks - When waiting for stuff to happen, set this to the number
 *   of clock ticks (at whatever clock frequency) to be the wakeup. Used
 *   by libradio_tick_wait() to figure out if the main_ticks clock has
 *  expired.
 * tick_count - This resets main_ticks to be this value, when main_ticks
 *   expires.
 * catch_irq - Should we listen for radio IRQs?
 *
 * my_channel - My channel number. Zero is the sleepy channel 
 * curr_channel - Current channel
 * my_node_id - My NodeID. Zero means "unset"
 * cat1, cat2 - Two category bytes (see README) 
 * num1,  num2 - Two instance bytes (see README)
 *
 * chip_rev - Si4463 chip revision
 * part_id - Si4463 part ID
 * pbuild - Si4463  part build version
 * device_id - Si4463 device Id
 * customer - Si4463 customer version
 * rom_id - Si4463 ROM ID
 * rev_ext - Si4463 revision extension
 * rev_branch - Si4463 revision branch
 * rev_int - Si4463 revision integer
 * patch - Si4463 patch-level 
 * func - Si4463 func-level
 *
 * npacket_rx - No. of good packets received
 * npacket_tx - No. of packets transmitted
 * curr_state - Current state
 * radio_active - Radio active
 * rx_fifo - RX fifo count
 * tx_fifo - TX fifo count
 * int_pending - interrupt-pending status
 * int_status - interrupt status
 * ph_pending - packet handler interrupt-pending status
 * ph_status - packet handler interrupt status
 * modem_pending - modem interrupt-pending status
 * modem_status - modem interrupt status
 * chip_pending - chip interrupt-pending status
 * chip_status - chip interrupt status
 * cmd_error - command error
 * saw_rx - Saw an RX packet (boolean)
 */
struct libradio {
	/*
	 * Overall state and time.
	 */
	uchar_t		state;
	uchar_t		tens_of_minutes;
	uint_t		ms_ticks;
	uchar_t		slow_period;
	uchar_t		fast_period;
	uchar_t		period;
	uint_t		heart_beat;
	uint_t		date;
	uint_t		timeout;
	uint_t		all_ticks;
	uint_t		main_ticks;
	uint_t		tick_count;
	uchar_t		catch_irq;
	/*
	 * Node and channel identification.
	 */
	uchar_t		my_channel;
	uchar_t		curr_channel;
	uchar_t		my_node_id;
	uchar_t		cat1, cat2;
	uchar_t		num1, num2;
	/*
	 * Radio settings.
	 */
	uchar_t		chip_rev;
	uint_t		part_id;
	uchar_t		pbuild;
	uint_t		device_id;
	uchar_t		customer;
	uchar_t		rom_id;
	uchar_t		rev_ext;
	uchar_t		rev_branch;
	uchar_t		rev_int;
	uint_t		patch;
	uchar_t		func;
	/*
	 * Radio parameters.
	 */
	uint_t		npacket_rx;
	uint_t		npacket_tx;
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
	uchar_t		saw_rx;
	uchar_t		current_rssi;
	uchar_t		latch_rssi;
	uchar_t		ant1_rssi;
	uchar_t		ant2_rssi;
};

extern uchar_t			pkt_data[MAX_SPI_BLOCK];
extern struct libradio	radio;

/*
 *
 */
void	pkt_init();
uchar_t	pkt_send(uchar_t, uchar_t);
uchar_t	libradio_rxpacket(struct channel *chp);
uchar_t	libradio_txpacket(struct channel *);
uchar_t	pkt_error(uchar_t);
void	libradio_set_song(uchar_t);
void	libradio_command(struct packet *);
void	libradio_send_response(uchar_t, uchar_t, uchar_t, uchar_t, uchar_t []);

void	_setss(uchar_t);
