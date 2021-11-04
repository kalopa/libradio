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

struct channel	recvchan;

void	handle_packet(struct libradio *);

/*
 *
 */
void
libradio_loop(struct libradio *rp)
{
	/*
	 * Handle the state dispensation, so we know what to do next.
	 */
	switch (rp->state) {
	case LIBRADIO_STATE_SLEEP:
	case LIBRADIO_STATE_SHUTDOWN:
	default:
		handle_packet(rp);
		break;

	case LIBRADIO_STATE_REBOOT:
	case LIBRADIO_STATE_LOWBATT:
	case LIBRADIO_STATE_ERROR:
		break;
	}
}

/*
 *
 */
void
handle_packet(struct libradio *rp)
{
	int i;
	struct packet *pp;
	struct channel *chp = &recvchan;

	/*
	 * Look for a received packet.
	 */
	puts("Go to receive...\n");
	if (libradio_recv(rp, chp, rp->my_channel)) {
		printf("\nPacket RX! (tick:%u),len:%d\n", rp->ms_ticks, chp->offset);
#if 0
		for (i = 0; i < chp->offset;) {
			pp = (struct packet *)&chp->payload[i];
			printf("N%d,L%d,C%d\n", pp->node, pp->len, pp->cmd);
			if (pp->len == 0)
				break;
			if (pp->node == 0 || pp->node == rp->my_node_id)
				libradio_command(rp, pp);
			i += pp->len;
		}
#endif
	}
}
