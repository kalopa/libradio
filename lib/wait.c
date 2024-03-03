/*
 * Copyright (c) 2019-24, Kalopa Robotics Limited.  All rights reserved.
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
 * The main client application just calls libradio_rxloop() in a while loop
 * and this does the bulk of the radio communications work. It manages the
 * state machine for the radio, and attempts to read (and process) any
 * received packets.
 */
#include <stdio.h>
#include <avr/io.h>

#include <libavr.h>

#include "libradio.h"
#include "internal.h"

/*
 * Track whenever the radio interrupts us.
 */
uchar_t		_irq_fired = 0;

/*
 * Wait for the timer to tick, an interrupt from the radio, or some serial
 * I/O. The intent here is to slow down the processor and to reduce the
 * amount of power consumed. We put the processor to sleep, while we wait.
 */
uchar_t
libradio_wait()
{
	while (!_irq_fired && !libradio_tick_wait() && sio_iqueue_empty()) {
		_watchdog();
		_sleep();
	}
	return(_irq_fired);
}

/*
 * An IRQ from the radio hits PD2 (INT0).  The falling edge causes
 * an interrupt. This function enables or disables radio interrupts.
 * The IRQ simply disables any further interrupts and sets _irq_fired,
 * which is used by the wait-timer.  We use catch_irq to track the fact
 * we're using radio IRQs. This is so we remember to re-enable them,
 * later on (see libradio_recv()).
 */
void
libradio_irq_enable(uchar_t flag)
{
	_irq_fired = 0;
	EICRA = 02;
	if ((radio.catch_irq = flag) == 0)
		EIMSK = 0;
	else
		EIMSK = 01;
}

/*
 * Return the status of the _irq_fired bit
 */
uchar_t
libradio_irq_fired()
{
	return(_irq_fired);
}
