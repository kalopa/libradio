/*
 * Copyright (c) 2019-21, Kalopa Robotics Limited.  All rights reserved.
 * Unpublished rights reserved under the copyright laws  of the Republic
 * of Ireland.
 *
 * The software contained herein is proprietary to and embodies the
 * confidential technology of Kalopa Robotics Limited.  Possession,
 * use, duplication or dissemination of the software and media is
 * authorized only pursuant to a valid written license from Kalopa
 * Robotics Limited.
 *
 * RESTRICTED RIGHTS LEGEND   Use, duplication, or disclosure by
 * the U.S.  Government is subject to restrictions as set forth in
 * Subparagraph (c)(1)(ii) of DFARS 252.227-7013, or in FAR 52.227-19,
 * as applicable.
 *
 * THIS SOFTWARE IS PROVIDED BY KALOPA ROBOTICS LIMITED "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL KALOPA ROBOTICS LIMITED
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ABSTRACT
 * This is where it all kicks off. Have fun, baby!
 */
#include <stdio.h>
#include <avr/io.h>

#include <libavr.h>

#include "libradio.h"
#include "radio.h"

/*
 * Handle a received command from the radio. We deal with the lower set of
 * commands (0->7 currently) internally, and pass any other commands on up
 * to the main system via the operate() function.
 */
void
libradio_command(struct libradio *rp, struct packet *pp)
{
	switch (pp->cmd) {
	printf("Received a command: %d\n", pp->cmd);
	case RADIO_CMD_FIRMWARE:
		break;

	case RADIO_CMD_STATUS:
		break;

	case RADIO_CMD_ACTIVATE:
		if (pp->len != 8)
			break;
		printf(">> Activate! %d%02d\n", rp->unique_code1, rp->unique_code2);
		if (pp->data[2] == rp->unique_code1 && pp->data[3] == rp->unique_code2) {
			rp->my_channel = pp->data[0];
			rp->my_node_id = pp->data[1];
			printf("ACTIVATED! [C%dN%d]\n", rp->my_channel, rp->my_node_id);
		}
		libradio_set_state(rp, LIBRADIO_STATE_ACTIVE);
		break;

	case RADIO_CMD_DEACTIVATE:
		printf(">> Deactivated...\n");
		libradio_set_state(rp, LIBRADIO_STATE_SHUTDOWN);
		break;

	case RADIO_CMD_SET_TIME:
		if (pp->len == 4)
			rp->tens_of_minutes = pp->data[0];
		printf(">> ToM:%u\n", rp->tens_of_minutes);
		break;

	case RADIO_CMD_SET_DATE:
		if (pp->len == 5)
			rp->date = (pp->data[1] << 8 | pp->data[0]);
		printf(">> Date:%u\n", rp->date);
		break;

	case RADIO_CMD_READ_EEPROM:
		printf("READ EEPROM\n");
		break;

	case RADIO_CMD_WRITE_EEPROM:
		printf("WRITE EEPROM\n");
		break;

	default:
		operate(rp, pp);
		break;
	}
}

/*
 *
 */
void
libradio_set_state(struct libradio *rp, uchar_t new_state)
{
	libradio_set_song(rp->state = new_state);
}
