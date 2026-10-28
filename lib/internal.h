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
 * This is the internal library for the libradio module. It isn't really
 * designed to be included by the main application. It should be able
 * to do what it needs, with the published functions. The internals of
 * the library are defined here.
 */
#define MAX_SPI_BLOCK				20

/*
 * We are not channel-hopping: every node talks on a single, fixed RF
 * frequency channel (a Si4463 concept). Don't confuse this with
 * struct packet's rxchan field, which is the logical per-device address.
 */
#define RF_CHANNEL					0

/*
 * TDMA slot timing. Timer1 runs free (Normal mode) at 250kHz (16MHz/64),
 * so one 20ms slot is 5000 ticks and 50 slots make up the 1-second
 * superframe. The /64 prescaler gives us 4us resolution for the latency
 * calibration below, and matches what the garden railway locomotive
 * already runs Timer1 at, so its wheel-encoder input capture (ICP1, on
 * this same counter) keeps the granularity it expects.
 *
 * Note the 16-bit counter wraps every 262ms at this rate. That's fine -
 * slot boundaries are always computed as a *relative* advance from the
 * last one, so the wraparound takes care of itself.
 */
#define SLOT_TICKS					5000
#define SLOT_MS						20

/*
 * Timing-compensation tunables, in Timer1 ticks (4us each). Both need
 * calibrating against the bench rig (master + client side by side, scope
 * on both) - these are starting estimates, not measured values.
 *
 * LIBRADIO_RX_SYNC_OFFSET - ticks to set our local clock to (instead of
 * zero) on receiving slot 0's beacon. By the time we've seen the beacon,
 * real time has moved on, and the dominant term is simply how long the
 * packet took to go out over the air: at 50kbps (see radio_config.h) a
 * beacon is 8 preamble + 2 sync + 16 data + 2 CRC = 28 bytes = 224 bits,
 * which is ~4.5ms of airtime. Add SPI readout of the FIFO (17 bytes at
 * 400kHz, ~340us) and ISR/poll overhead, and ~5ms is the right ballpark.
 * 5ms / 4us = 1250 ticks.
 *
 * LIBRADIO_TX_LEAD_TICKS - how many ticks *before* our assigned slot's
 * boundary slot_func() fires, so the app has time to stage its packet via
 * libradio_transmit() well ahead of the actual transmit trigger, rather
 * than being asked for data at the last possible moment. 800 ticks is
 * 3.2ms, comfortably more than the ~340us the FIFO load itself needs.
 */
#define LIBRADIO_RX_SYNC_OFFSET		1250
#define LIBRADIO_TX_LEAD_TICKS		800

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
 * my_id - My NodeID. Zero means "unset"
 * cat1, cat2 - Two category bytes (see README)
 * num1, num2 - Two instance bytes (see README)
 * heart_beat - Timer used for the LED song
 * millis, hhmm - Milliseconds and minutes-since-midnight (see clock.c)
 * curr_slot - Which of the LIBRADIO_NO_SLOTS TDMA slots we're in
 * tx_pending - A packet is staged in the TX FIFO, awaiting its slot
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
	uchar_t			state;
	uchar_t			my_id;
	uchar_t			cat1, cat2;
	uchar_t			num1, num2;
	uint_t			heart_beat;
	uint_t			millis;
	uint_t			hhmm;
	/*
	 * TDMA slot tracking. curr_slot is the slot we're currently in
	 * (0..LIBRADIO_NO_SLOTS-1). Which slot(s) "belong" to this node is
	 * an application-level decision (see slot_func()'s slotid argument)
	 * - the library just fires the callback once per slot and lets the
	 * app decide what, if anything, to do with it.
	 */
	uchar_t			curr_slot;
	/*
	 * tx_pending - a packet has been staged into the TX FIFO (via
	 * libradio_transmit()) and is waiting for the slot boundary to
	 * actually fire SI4463_START_TX.
	 */
	uchar_t			tx_pending;
	/*
	 * Callbacks and packet queue.
	 */
	void			(*cmd_func)(struct packet *);
	void			(*slot_func)(int);
	struct packet	*txqueue;
	/*
	 * Radio settings.
	 */
	uchar_t			chip_rev;
	uint_t			part_id;
	uchar_t			pbuild;
	uint_t			device_id;
	uchar_t			customer;
	uchar_t			rom_id;
	uchar_t			rev_ext;
	uchar_t			rev_branch;
	uchar_t			rev_int;
	uint_t			patch;
	uchar_t			func;
	/*
	 * Radio parameters.
	 */
	uint_t			npacket_rx;
	uint_t			npacket_tx;
	uchar_t			curr_state;
	uchar_t			radio_active;
	uchar_t			rx_fifo;
	uchar_t			tx_fifo;
	uchar_t			int_pending;
	uchar_t			int_status;
	uchar_t			ph_pending;
	uchar_t			ph_status;
	uchar_t			modem_pending;
	uchar_t			modem_status;
	uchar_t			chip_pending;
	uchar_t			chip_status;
	uchar_t			cmd_error;
	uchar_t			saw_rx;
	uchar_t			current_rssi;
	uchar_t			latch_rssi;
	uchar_t			ant1_rssi;
	uchar_t			ant2_rssi;
};

extern struct libradio	radio;
extern uint_t		_slot_prev;
extern uint_t		_slot_next;
extern uint_t		_slot_boundary_tick;
extern uchar_t		_irq_fired;
extern uchar_t		pkt_data[MAX_SPI_BLOCK];

void			clock_init();
void			clocktimer();
void			slot_init();
uint_t			libradio_get_prev_slot();
void			libradio_set_next_slot(uint_t);
void			libradio_slot_advance();
void			libradio_sync_clock();

/*
 * Raw SI4463 command-transaction layer (lib/spi.c). Send/receive framing
 * for the chip's command protocol - knows nothing about our packet format.
 */
void	pkt_init();
uchar_t	pkt_send(uchar_t, uchar_t);
uchar_t	pkt_error(uchar_t);

/*
 * Chip status/property helpers (lib/radio.c).
 */
int		libradio_request_device_status();
void	libradio_get_part_info();
void	libradio_get_func_info();
void	libradio_get_chip_status();
void	libradio_get_int_status();
uchar_t	libradio_get_fifo_info(uchar_t);
void	libradio_ircal();
void	libradio_change_radio_state(uchar_t);

/*
 * Power up/down the radio chip itself (lib/power.c).
 */
int		libradio_power_up();
void	libradio_power_down();

/*
 * Packet TX/RX over the FIFO (lib/xmit.c, lib/recv.c). No checksum - the
 * Si4463's own hardware CRC (see RF_PKT_CRC_CONFIG_7) validates packets.
 */
uchar_t	libradio_txpacket(struct packet *);
void	libradio_fire_tx();
uchar_t	libradio_rxpacket(struct packet *);
void	libradio_set_rx();
uchar_t	libradio_check_rx();

/*
 * Radio IRQ enable/disable (a landed packet raises PD2/INT0).
 */
void	libradio_irq_enable(uchar_t);

void	_setss(uchar_t);
void	_testpt(uchar_t);

/*
 * Scope instrumentation on the test point (PC5 - see testpt.S; it's a
 * plain AVR pin, so nothing in radio_config.h needs touching). Set this
 * to 0 to compile it out entirely.
 *
 * The transmitter raises it around the START_TX command, and the receiver
 * raises it while pulling a packet out of the FIFO. With a probe on each
 * board, the gap from the master's rising edge to the client's rising
 * edge is exactly the latency LIBRADIO_RX_SYNC_OFFSET has to cancel out.
 * The pulse widths are informative too: expect ~60us for the START_TX
 * command and ~340us for the 17-byte FIFO readout at 400kHz.
 */
#define LIBRADIO_TESTPT				1

#if LIBRADIO_TESTPT
#define TESTPT(x)	_testpt(x)
#else
#define TESTPT(x)
#endif
