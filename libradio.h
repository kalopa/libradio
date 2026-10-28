/*
 * Copyright (c) 2020-25, Kalopa Robotics Limited.  All rights reserved.
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
 *
 * HARDWARE RESOURCES OWNED BY THIS LIBRARY
 * Do not configure these in your application - libradio_init() sets them
 * up and anything you set beforehand is clobbered:
 *
 *   TIMER1 - the 20ms TDMA slot clock (Compare Match A). Note that the
 *            input capture unit is NOT used by us, so an application is
 *            free to use ICP1 (e.g. a wheel encoder) on the same counter.
 *   TIMER0 - the real-time clock and LED heartbeat "song".
 *   INT0   - the radio's IRQ line (PD2).
 *   SPI    - shared with the radio; see lib/spi.c.
 *
 * TIMER2 is left entirely to the application. Note that TIMER0's claim
 * is awkward for any device needing hardware PWM on OC0A/OC0B (a motor
 * controller, say) - see the notes in lib/clock.c.
 */
#define MAX_FIFO_SIZE		48
#define MAX_PACKET_SIZE		16
#define MAX_PAYLOAD_SIZE	14

/*
 * State machine used to manage radio operations.
 *
 * ASLEEP    - Not listening to the radio at all.
 * LISTENING - Powered up, RX on, haven't heard a beacon yet.
 * SYNCED    - Heard a beacon (clock aligned to the team lead) but have no
 *             slot assignment yet.
 * ONLINE    - Assigned a slot and actively communicating. Reserves the
 *             0x00f7 heartbeat song - don't reuse it for any other state.
 * TEAMLEAD  - This node *is* the team lead.
 */
#define LIBRADIO_STATE_ASLEEP		0
#define LIBRADIO_STATE_LISTENING	1
#define LIBRADIO_STATE_SYNCED		2
#define LIBRADIO_STATE_ONLINE		3
#define LIBRADIO_STATE_TEAMLEAD		4
#define LIBRADIO_NSTATES		5

/*
 * Default channel assignments
 */
#define LIBRADIO_CHANNEL_TEAMLEAD	0
#define LIBRADIO_CHANNEL_BROADCAST	255

/*
 * Standard command set is the first 16 commands.
 */
#define RADIO_CMD_BEACON			0x00
#define RADIO_CMD_TIMESTAMP			0x01
#define RADIO_CMD_HELLO				0x02
#define RADIO_CMD_REQUEST_ID		0x03
#define RADIO_CMD_DEVICE_STATUS		0x04
#define RADIO_CMD_STATUS_RESPONSE	0x05

/*
 * Slot map. One superframe is LIBRADIO_NO_SLOTS slots of 20ms each (1s
 * total). Slot 0 is always the team lead's BEACON. Slots 12/24/36 are
 * reserved for the team lead to relay LEADER_CMD traffic (commands/data
 * from the upstream teamlink host) - time/date sync piggybacks on these
 * rather than getting its own dedicated slot. Slots 48/49 (the last two)
 * are ALOHA slots for unassigned devices to request a channel. Every
 * other slot (44 of them) is assignable to a client device.
 */
#define LIBRADIO_SLOT_BEACON		0
#define LIBRADIO_SLOT_LEADER_CMD1	12
#define LIBRADIO_SLOT_LEADER_CMD2	24
#define LIBRADIO_SLOT_LEADER_CMD3	36
#define LIBRADIO_SLOT_ALOHA1		48
#define LIBRADIO_SLOT_ALOHA2		49
#define LIBRADIO_NO_SLOTS			50

/*
 * Additional application-specific commands.
 */
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

/*
 * Packet to be transmitted.
 */
struct packet	{
	uchar_t		rxchan;					/* ID for receiver (0xFF is a broadcast) */
	uchar_t		command;				/* Command for the receiver */
	uchar_t		data[MAX_PAYLOAD_SIZE];
};

/*
 * Prototypes...
 */
void		libradio_init(uchar_t, uchar_t, uchar_t, uchar_t);
void		libradio_command_callback(void (*)(struct packet *));
void		libradio_slot_callback(void (*)(int));
void		libradio_teamlead();
void		libradio_poll();
int			libradio_transmit(struct packet *);
void		libradio_set_state(int);
void		libradio_set_latency(int);
void		libradio_set_window(int, int);
uchar_t		*libradio_get_assignments();
uint_t		libradio_millis();
uint_t		libradio_hhmm();
