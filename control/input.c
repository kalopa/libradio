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
 * This is the operational code (and state machine) for the main
 * controller.
 */
#include <stdio.h>
#include <avr/io.h>

#include <libavr.h>

#include "libradio.h"
#include "control.h"

#define IO_STATE_NEWLINE		0
#define IO_STATE_WAITNL			1
#define IO_STATE_WAITCHAN		2
#define IO_STATE_WAITNODE		3
#define IO_STATE_WAITCMD		4
#define IO_STATE_WAITDATA		5

#define STATE(s, ch)			((ch) << 3 | (s))

uchar_t			state = IO_STATE_NEWLINE;
uchar_t			value;
struct channel	*curr_chp;

/*
 *
 */
void
process_input()
{
	uchar_t ch = getchar();

	/*
	 * A carriage-return or newline is a good way to flush out any junk and
	 * get to a known state on the serial input.
	 */
	if (ch == '\n' || ch == '\r') {
		if (state >= IO_STATE_WAITCMD)
			enqueue(curr_chp);
		state = IO_STATE_NEWLINE;
		return;
	}
	/*
	 * Otherwise, use a state machine to handle character input.
	 */
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
		if ((value = ch - 'A') < 0 || value >= MAX_RADIO_CHANNELS) {
			/*
			 * Invalid channel - abort!
			 */
			reply(RADIO_CTLERR_INVALID_CHANNEL);
			break;
		}
		curr_chp = &channels[value];
		if (curr_chp->state > LIBRADIO_CHSTATE_EMPTY) {
			/*
			 * Too much data for this channel. Abort!
			 */
			reply(RADIO_CTLERR_BUSY);
			break;
		}
		curr_chp->state = LIBRADIO_CHSTATE_ADDING;
		state = IO_STATE_WAITNODE;
		value = 0;
		break;

	case STATE(IO_STATE_WAITCHAN, 'R'):
		_reset();
		break;

	case STATE(IO_STATE_WAITCHAN, 'S'):
	case STATE(IO_STATE_WAITCHAN, 'T'):
		local_status(ch - 'S');
		state = IO_STATE_NEWLINE;
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
		curr_chp->packet.node = value;
		value = 0;
		break;

	case STATE(IO_STATE_WAITCMD, ':'):
	case STATE(IO_STATE_WAITCMD, '.'):
		curr_chp->packet.cmd = value;
		curr_chp->packet.len = 0;
		value = 0;
		if (ch == '.') {
			enqueue(curr_chp);
			state = IO_STATE_WAITNL;
		} else
			state = IO_STATE_WAITDATA;
		break;

	case STATE(IO_STATE_WAITDATA, ','):
	case STATE(IO_STATE_WAITDATA, '.'):
		if (curr_chp->packet.len >= MAX_PAYLOAD_SIZE) {
			/*
			 * Too much data for this channel. Abort! Note that we reserve
			 * two bytes to specify node:0,len:0 at the end.
			 */
			reply(RADIO_CTLERR_TOO_BIG);
			break;
		}
		curr_chp->packet.data[curr_chp->packet.len++] = value;
		value = 0;
		if (ch == '.') {
			enqueue(curr_chp);
			state = IO_STATE_WAITNL;
		}
		break;

	default:
		if (state != IO_STATE_WAITNL)
			reply(RADIO_CTLERR_NEWLINE);
		break;
	}
}

/*
 *
 */
void
reply(uchar_t code)
{
	if (code == 0)
		printf("<+\n");
	else
		printf("<-%d/%d\n", code, libradio_get_state());
	state = IO_STATE_WAITNL;
}
