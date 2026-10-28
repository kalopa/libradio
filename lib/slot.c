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
 * The TDMA slot scheduler. Timer1 (see clock_init() in clock.c and t1int
 * in locore.S) fires a compare-match interrupt at each slot boundary; the
 * ISR just latches TCNT1 into _slot_prev and reloads OCR1A from
 * _slot_next. Everything else - stepping through the 50-slot superframe,
 * and keeping the timer firing at a steady 20ms cadence - happens here in
 * C, driven from poll.c.
 */
#include <stdio.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

#include <libavr.h>

#include "libradio.h"
#include "internal.h"

uint_t		_slot_prev;
uint_t		_slot_next;
uint_t		_slot_boundary_tick;
uchar_t		assignments[MAX_PAYLOAD_SIZE];

/*
 * Initialize the slots
 */
void
slot_init()
{
	int i;

	for (i = 0; i < MAX_PAYLOAD_SIZE; i++)
		assignments[i] = 0;
	radio.curr_slot = 0;
	_slot_prev = 0xffff;
	_slot_next = SLOT_TICKS;
	_slot_boundary_tick = 0;
}

/*
 * Get the previous slot (0xffff if none)
 */
uint_t
libradio_get_prev_slot()
{
	return(_slot_prev);
}

/*
 * Set the next slot time
 */
void
libradio_set_next_slot(uint_t next)
{
	_slot_next = next;
}

/*
 * Register a slot callback.
 */
void
libradio_slot_callback(void (*func)(int))
{
	radio.slot_func = func;
}

/*
 * Return the assignments list.
 */
uchar_t	*
libradio_get_assignments()
{
	return(assignments);
}

/*
 * A slot boundary has just fired (poll.c saw _slot_prev != 0xffff). Step
 * to the next slot in the superframe, and keep Timer1 running at a steady
 * SLOT_TICKS cadence by requesting the next compare match relative to
 * where we actually just fired - not a fixed absolute value, which would
 * only refire on a full 16-bit counter wraparound rather than every slot.
 */
void
libradio_slot_advance()
{
	_slot_boundary_tick = _slot_prev;
	libradio_set_next_slot(_slot_prev + SLOT_TICKS);
	radio.curr_slot = (radio.curr_slot + 1) % LIBRADIO_NO_SLOTS;
	_slot_prev = 0xffff;
	/*
	 * The 20ms slot tick is also our system heartbeat now that TIMER0
	 * belongs to the application - update the clock and the LED song.
	 */
	clocktimer();
}

/*
 * Re-align our local slot clock to the team lead's, on receipt of a
 * BEACON. By the time we see it, real time has already moved on by
 * however long it took the team lead to transmit it, the two radios'
 * latency, and our own ISR overhead - so we don't set the clock to zero,
 * we set it to LIBRADIO_RX_SYNC_OFFSET (calibrated on the bench).
 */
void
libradio_sync_clock()
{
	cli();
	TCNT1 = LIBRADIO_RX_SYNC_OFFSET;
	OCR1A = LIBRADIO_RX_SYNC_OFFSET + SLOT_TICKS;
	sei();
	_slot_boundary_tick = LIBRADIO_RX_SYNC_OFFSET;
	_slot_prev = 0xffff;
	_slot_next = LIBRADIO_RX_SYNC_OFFSET + SLOT_TICKS;
	radio.curr_slot = 0;
	if (radio.state == LIBRADIO_STATE_LISTENING)
		libradio_set_state(LIBRADIO_STATE_SYNCED);
}
