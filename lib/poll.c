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
 * libradio_poll() is the non-blocking heart of the library - called
 * repeatedly from the app's main loop, it never sleeps and never waits.
 * Each call:
 *
 * 1. If a slot boundary just fired (Timer1 ISR, see locore.S), fires any
 *    staged transmission immediately (the only truly time-critical SPI
 *    command), then steps the scheduler to the next slot.
 * 2. If the radio IRQ pin fired, a packet has landed - pull it out of the
 *    FIFO and hand it to the app's command callback.
 * 3. If we're within LIBRADIO_TX_LEAD_TICKS of the *next* slot boundary,
 *    fire the app's slot callback once so it can stage a packet (via
 *    libradio_transmit()) well ahead of time, rather than being asked for
 *    data at the last possible moment.
 */
#include <stdio.h>
#include <avr/io.h>
#include <string.h>

#include <libavr.h>

#include "libradio.h"
#include "internal.h"

uchar_t		_irq_fired = 0;

static uchar_t	lead_fired = 0;

/*
 * Look to see if there's anything to do. Don't block.
 */
void
libradio_poll()
{
	uchar_t next_slot;
	uint_t ticks_in_slot;

	if (_slot_prev != 0xffff) {
		libradio_fire_tx();
		libradio_slot_advance();
		lead_fired = 0;
	}
	if (_irq_fired) {
		struct packet pkt;
		uchar_t gotpkt;

		_irq_fired = 0;
		/*
		 * Raise the test point for exactly as long as it takes to get
		 * the packet out of the FIFO - the rising edge is "packet in
		 * hand", which is what we measure the beacon latency against.
		 */
		TESTPT(1);
		gotpkt = libradio_check_rx() ? libradio_rxpacket(&pkt) : 0;
		TESTPT(0);
		if (gotpkt) {
			if (pkt.command == RADIO_CMD_BEACON &&
					radio.state != LIBRADIO_STATE_TEAMLEAD)
				libradio_sync_clock();
			if (radio.cmd_func)
				radio.cmd_func(&pkt);
		}
		libradio_irq_enable(1);
	}
	/*
	 * Don't fire slot callbacks (and risk an uncoordinated transmission)
	 * until we're at least synced to the team lead's beacon - the team
	 * lead itself is always "synced" to its own clock, by definition.
	 */
	if (!lead_fired && radio.slot_func != 0 &&
			(radio.state == LIBRADIO_STATE_TEAMLEAD ||
			 radio.state == LIBRADIO_STATE_SYNCED ||
			 radio.state == LIBRADIO_STATE_ONLINE)) {
		ticks_in_slot = (uint_t )(TCNT1 - _slot_boundary_tick);
		if (ticks_in_slot >= SLOT_TICKS - LIBRADIO_TX_LEAD_TICKS) {
			next_slot = (radio.curr_slot + 1) % LIBRADIO_NO_SLOTS;
			radio.slot_func(next_slot);
			lead_fired = 1;
		}
	}
}

/*
 * Set the operational state.
 */
void
libradio_set_state(int newstate)
{
	radio.state = newstate;
}

/*
 * An IRQ from the radio hits PD2 (INT0). The falling edge causes an
 * interrupt (see radio_irq in locore.S, which just latches _irq_fired and
 * masks further interrupts until we re-enable them here).
 */
void
libradio_irq_enable(uchar_t flag)
{
	EICRA &= ~03;
	EICRA |= 02;
	if (flag == 0) {
		EIMSK &= ~01;
		_irq_fired = 0;
	} else {
		_irq_fired = 0;
		EIMSK |= 01;
	}
}
