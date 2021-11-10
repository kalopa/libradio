/*
 * Copyright (c) 2019-21, Kalopa Robotics Limited.  All rights reserved.
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
 * The main application just calls libradio_loop() in the main while loop
 * and this does the bulk of the radio communications work. It manages
 * the state machine for the radio, and attempts to read (and process)
 * any received packets. The function to set the system state lives here,
 * too. So all of the mechanics of the state machine are defined in here.
 */
#include <stdio.h>
#include <avr/io.h>

#include <libavr.h>

#include "libradio.h"
#include "internal.h"

struct channel	recvchan;

void	handle_packet();

/*
 * This is called repeatedly from the main loop. The task here is based on
 * the current state. Note we will halt the processor if there is nothing
 * to do at this point.
 */
void
libradio_loop()
{
	/*
	 * First things first - wait for the clock timer to kick us into doing
	 * something. The sleep() function will pause the CPU until the next IRQ.
	 */
	while (radio.main_thread == 0)
		_sleep();
	putchar('+');
	/*
	 * Depending on what state we're in, do something useful. For a lot of
	 * these states, not much happens and all we do is move to the next
	 * state.
	 */
	switch (radio.state) {
	case LIBRADIO_STATE_ERROR:
		/*
		 * We are in an error state, for a full minute. Go to a WARM sleep.
		 */
		libradio_set_state(LIBRADIO_STATE_WARM);
		break;

	case LIBRADIO_STATE_STARTUP:
	case LIBRADIO_STATE_COLD:
	case LIBRADIO_STATE_WARM:
		/*
		 * Time to wake up and check for radio traffic.
		 */
		libradio_set_state(LIBRADIO_STATE_LISTEN);
		break;

	case LIBRADIO_STATE_LISTEN:
		handle_packet();
#if 0
		if (--radio.timeout == 0) {
			/*
			 * If we're still in this state after the timeout, then we go to
			 * WARM or COLD sleep depending on whether we've seen any traffic.
			 */
			libradio_set_state(radio.saw_rx ? LIBRADIO_STATE_WARM :
												LIBRADIO_STATE_COLD);
		}
#else
		radio.main_ticks = 10;
#endif
		break;

	default:
		/*
		 * Any active state - check for radio traffic.
		 */
		handle_packet();
		if (radio.saw_rx) {
			radio.timeout = 60000;
			radio.saw_rx = 0;
		}
		if (--radio.timeout == 0) {
			/*
			 * Haven't seen any traffic in a *long* time.
			 */
			libradio_set_state(LIBRADIO_STATE_WARM);
		}
		break;
	}
	radio.main_thread = 0;
}

/*
 * Helper function to return the current state.
 */
uchar_t
libradio_get_state()
{
	return(radio.state);
}

/*
 * Update the state and the associated parameters. Note that this should be
 * idempotent in that it will not change anything unless there is an actual
 * state change.
 */
void
libradio_set_state(uchar_t new_state)
{
	long wait_ticks = 1;

	if (new_state != LIBRADIO_STATE_STARTUP && radio.state == new_state)
		return;
	libradio_set_song(new_state);
	switch (new_state) {
	case LIBRADIO_STATE_COLD:
	case LIBRADIO_STATE_WARM:
		/*
		 * Time to reduce power and wait for a while. Also turn off the real
		 * time clock - no point trying to track the time in this mode. Wait
		 * for 15 or 60 minutes depending.
		 */
		power_mode(0);
		radio.tens_of_minutes = 0xff;
		wait_ticks = (new_state == LIBRADIO_STATE_WARM) ?
										15*60*1000L : 60*60*1000L;
		wait_ticks /= (long )radio.slow_period;
		break;

	case LIBRADIO_STATE_LISTEN:
		/*
		 * Listen for radio activity for one minute.
		 */
		if (radio.state == LIBRADIO_STATE_COLD ||
						radio.state == LIBRADIO_STATE_WARM) {
			/*
			 * Go into high power mode while in this state. Just make sure
			 * that we were already in low power mode, or we'll glitch the
			 * real-time clock and the time of day.
			 */
			power_mode(1);
		}
		/*
		 * Switch to channel 0, clear the packet flag, and set a timeout
		 * for sixty seconds. We need to see something in that period,
		 * or else we're going for a nice, long sleep...
		 */
		radio.my_channel = 0;
		radio.saw_rx = 0;
		if (radio.state == LIBRADIO_STATE_COLD)
			radio.timeout = 60000 / radio.fast_period;
		else
			radio.timeout = (int )(120000L / (long )radio.fast_period);
		break;

	case LIBRADIO_STATE_ERROR:
		/*
		 * Some sort of fatal error. Stay in this state for ten minutes.
		 */
		wait_ticks = 10*60*1000L / (long )radio.fast_period;
		break;

	default:
		/*
		 * All of the active states. The thing here is to rewind the RX
		 * timeout for 60,000 passes through the loop (whatever that is
		 * in seconds).
		 */
		radio.timeout = 60000;
		break;
	}
	cli();
	radio.main_ticks = (int )wait_ticks;
	radio.state = new_state;
	sei();
	printf("RS:%d,mt%d\n", radio.state, radio.main_ticks);
}

/*
 *
 */
void
handle_packet()
{
	int i;
	struct packet *pp;
	struct channel *chp = &recvchan;

	/*
	 * Look for a received packet.
	 */
	printf("HP:%d\n", radio.my_channel);
	if (libradio_recv(chp, radio.my_channel)) {
		printf("\nPacket RX! (tick:%u),len:%d\n", radio.ms_ticks, chp->offset);
		for (i = 0; i < chp->offset;) {
			pp = (struct packet *)&chp->payload[i];
			printf("N%d,L%d,C%d\n", pp->node, pp->len, pp->cmd);
			if (pp->len == 0)
				break;
			if (pp->node == 0 || pp->node == radio.my_node_id)
				libradio_command(pp);
			i += pp->len;
		}
	}
}
