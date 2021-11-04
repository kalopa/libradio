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
 * This is where it all kicks off. Have fun, baby!
 */
#include <stdio.h>
#include <avr/io.h>

#include <libavr.h>

#include "libradio.h"

#define NPRIORITIES				25

#define IO_STATE_NEWLINE		0
#define IO_STATE_WAITNL			1
#define IO_STATE_WAITCHAN		2
#define IO_STATE_WAITNODE		3
#define IO_STATE_WAITCMD		4
#define IO_STATE_WAITDATA		5

#define STATE(s, ch)			((ch) << 3 | (s))

uchar_t			channel_prio;
uchar_t			state = IO_STATE_NEWLINE;
uchar_t			value;
struct channel	channels[MAX_CHANNELS], *chp;
struct packet	*pp;

uchar_t priorities[NPRIORITIES] = {0,1,3,1,2,1,3,4,2,1,3,1,2,1,4,1,2,4,1,3,1,2,1,2,5};

uchar_t		execute(struct packet *);
void		send_time(struct channel *);
void		enqueue();

/*
 * Initialize operations. We send time stamps on each channel in and around the
 * even second, and twice per ten minute interval.
 */
void
opinit()
{
	int i;

	channel_prio = 0;
	for (i = 0; i < MAX_CHANNELS; i++)
		channels[i].state = CHANNEL_STATE_EMPTY;
}

/*
 * Transmit a single packet, if one is available in the queue.
 */
void
do_transmit()
{
	int i, channo;
	struct channel *chp;

	/*
	 * Every ten seconds, send a time stamp on each of the active channels.
	 */
	if ((ticks % 1000) == 0) {
		i = (ticks / 1000) % MAX_CHANNELS;
		chp = &channels[i];
		if (chp->state != CHANNEL_STATE_ADDING &&
								chp->offset < (MAX_PAYLOAD - 8))
			send_time(chp);
	}
	/*
	 * Run through the priorities table to see which channel we'll do now.
	 * The idea here is to increase the number of packets on channel 1,
	 * less so on channel 2, and rarely send anything on channel 0. But it
	 * is important to find an active packet so this really only comes into
	 * play when the system is very busy.
	 */
	for (i = 0; i < NPRIORITIES; i++) {
		channo = priorities[channel_prio++];
		if (channel_prio >= NPRIORITIES)
			channel_prio = 0;
		chp = &channels[channo];
		if (chp->state == CHANNEL_STATE_TRANSMIT)
			break;
	}
	if (i == NPRIORITIES)
		return;
	/*
	 * We have a channel ready for transmission. Send it now...
	 */
	chp->payload[chp->offset++] = 0;
	chp->payload[chp->offset++] = 0;
	if (radio_send(chp, channo) != 0)
		chp->state = CHANNEL_STATE_EMPTY;
}

/*
 * Add the packet to the queue. Set the state to TRANSMIT if we're ready to
 * send. Deal with the special-case where this is addressed to us and we
 * don't need to transmit it.
 */
void
enqueue()
{
	if (pp->cmd == COMMAND_MASTER_ACTIVATE || pp->node == radio.my_node_id) {
		/*
		 * This packet is for me. Don't queue it up for transmission.
		 * Instead, execute the command. Also, free up the channel buffer
		 * if this is the only transmission.
		 */
		status(execute(pp));
		chp->state = (chp->offset == 0) ? CHANNEL_STATE_EMPTY : CHANNEL_STATE_TRANSMIT;
		return;
	}
	/*
	 * Packet is for transmission. Update the channel offset. Note that we
	 * will only accept packets for transmission in an ACTIVE state.
	 */
	if (system_state == SYSTEM_STATE_ACTIVE) {
		/*
		 * Add the node, length and channel to the length. Then update
		 * the overall offset.
		 */
		pp->len += 3;
		chp->offset += pp->len;
		chp->state = CHANNEL_STATE_TRANSMIT;
		status(1);
	} else {
		chp->state = CHANNEL_STATE_EMPTY;
		status(0);
	}
}

/*
 * Send the current time of day to the specified channel.
 */
void
send_time(struct channel *chp)
{
	struct packet *pp;

	if (chp->state == CHANNEL_STATE_EMPTY)
		chp->offset = 0;
	chp->state = CHANNEL_STATE_TRANSMIT;
	pp = (struct packet *)&chp->payload[chp->offset];
	pp->node = 0;
	pp->len = 4;
	pp->cmd = COMMAND_SET_TIME;
	pp->data[0] = tens_of_minutes;
	chp->offset += 4;
}

/*
 *
 */
void
process_input()
{
	uchar_t ch = getchar();

	if (ch == '\n' || ch == '\r') {
		if (state >= IO_STATE_WAITCMD)
			enqueue();
		state = IO_STATE_NEWLINE;
		return;
	}
	switch (STATE(state, ch)) {
	case STATE(IO_STATE_NEWLINE, '>'):
		state = IO_STATE_WAITCHAN;
		break;

	case STATE(IO_STATE_WAITCHAN, 'A'):
	case STATE(IO_STATE_WAITCHAN, 'B'):
	case STATE(IO_STATE_WAITCHAN, 'C'):
	case STATE(IO_STATE_WAITCHAN, 'D'):
	case STATE(IO_STATE_WAITCHAN, 'E'):
	case STATE(IO_STATE_WAITCHAN, 'F'):
	case STATE(IO_STATE_WAITCHAN, 'G'):
	case STATE(IO_STATE_WAITCHAN, 'H'):
		/*
		 * We have a channel ID. Check it is within range.
		 */
		if ((value = ch - 'A') < 0 || value >= MAX_CHANNELS) {
			/*
			 * Invalid channel - abort!
			 */
			status(0);
			break;
		}
		chp = &channels[value];
		if (chp->state == CHANNEL_STATE_EMPTY)
			chp->offset = 0;
		else {
			if (chp->offset + 3 >= MAX_PAYLOAD) {
				/*
				 * Too much data for this channel. Abort!
				 */
				chp->state = CHANNEL_STATE_TRANSMIT;
				status(0);
				break;
			}
		}
		chp->state = CHANNEL_STATE_ADDING;
		pp = (struct packet *)&chp->payload[chp->offset];
		state = IO_STATE_WAITNODE;
		value = 0;
		break;

	case STATE(IO_STATE_WAITCHAN, 'R'):
		//_reset();
		break;

	case STATE(IO_STATE_WAITNODE, '0'):
	case STATE(IO_STATE_WAITNODE, '1'):
	case STATE(IO_STATE_WAITNODE, '2'):
	case STATE(IO_STATE_WAITNODE, '3'):
	case STATE(IO_STATE_WAITNODE, '4'):
	case STATE(IO_STATE_WAITNODE, '5'):
	case STATE(IO_STATE_WAITNODE, '6'):
	case STATE(IO_STATE_WAITNODE, '7'):
	case STATE(IO_STATE_WAITNODE, '8'):
	case STATE(IO_STATE_WAITNODE, '9'):
	case STATE(IO_STATE_WAITCMD, '0'):
	case STATE(IO_STATE_WAITCMD, '1'):
	case STATE(IO_STATE_WAITCMD, '2'):
	case STATE(IO_STATE_WAITCMD, '3'):
	case STATE(IO_STATE_WAITCMD, '4'):
	case STATE(IO_STATE_WAITCMD, '5'):
	case STATE(IO_STATE_WAITCMD, '6'):
	case STATE(IO_STATE_WAITCMD, '7'):
	case STATE(IO_STATE_WAITCMD, '8'):
	case STATE(IO_STATE_WAITCMD, '9'):
	case STATE(IO_STATE_WAITDATA, '0'):
	case STATE(IO_STATE_WAITDATA, '1'):
	case STATE(IO_STATE_WAITDATA, '2'):
	case STATE(IO_STATE_WAITDATA, '3'):
	case STATE(IO_STATE_WAITDATA, '4'):
	case STATE(IO_STATE_WAITDATA, '5'):
	case STATE(IO_STATE_WAITDATA, '6'):
	case STATE(IO_STATE_WAITDATA, '7'):
	case STATE(IO_STATE_WAITDATA, '8'):
	case STATE(IO_STATE_WAITDATA, '9'):
		value = (value * 10) + ch - '0';
		break;

	case STATE(IO_STATE_WAITNODE, ':'):
		state = IO_STATE_WAITCMD;
		pp->node = value;
		value = 0;
		break;

	case STATE(IO_STATE_WAITCMD, ':'):
	case STATE(IO_STATE_WAITCMD, '.'):
		state = IO_STATE_WAITDATA;
		pp->cmd = value;
		pp->len = 0;
		value = 0;
		if (ch == '.') {
			enqueue();
			state = IO_STATE_WAITNL;
		}
		break;

	case STATE(IO_STATE_WAITDATA, ','):
	case STATE(IO_STATE_WAITDATA, '.'):
		if ((chp->offset + pp->len) >= (MAX_PAYLOAD - 2)) {
			/*
			 * Too much data for this channel. Abort! Note that we reserve
			 * two bytes to specify node:0,len:0 at the end.
			 */
			status(0);
			break;
		}
		pp->data[pp->len++] = value;
		value = 0;
		if (ch == '.') {
			enqueue();
			state = IO_STATE_WAITNL;
		}
		break;

	default:
		if (state != IO_STATE_WAITNL)
			status(0);
		break;
	}
}

/*
 *
 */
void
status(uchar_t success)
{
	putchar('<');
	putchar(success ? '+' : '-');
	putchar('\n');
	state = IO_STATE_WAITNL;
}
