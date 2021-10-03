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
 * This helper function displays a 
 */
#include <stdio.h>
#include <avr/io.h>

#include <libavr.h>

#include "libradio.h"

uint_t	songs[NLIBRADIO_STATES] = {0xfff, 0, 0, 0xf, 0x5555, 0xf7};

volatile uint_t		hbeat;

/*
 * This function is called to set the heartbeat LED appropriately.
 */
void
libradio_heartbeat()
{
	uchar_t ledf;

	ledf = (hbeat & 0x8000) ? 1 : 0;
	_setled(ledf);
	hbeat = (hbeat << 1) | ledf;
}

/*
 * Set the LED "song" based on the state.
 */
void
libradio_set_song(uchar_t new_state)
{
	if (new_state <= LIBRADIO_STATE_ACTIVE)
		hbeat = songs[new_state];
	else
		hbeat = songs[LIBRADIO_STATE_ACTIVE];
}
