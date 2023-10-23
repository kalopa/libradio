;
; Copyright (c) 2020-23, Kalopa Robotics Limited.  All rights
; reserved.
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions
; are met:
;
; 1. Redistributions of source code must retain the above copyright
; notice, this list of conditions and the following disclaimer.
;
; 2. Redistributions in binary form must reproduce the above
;    copyright notice, this list of conditions and the following
;    disclaimer in the documentation and/or other materials provided
;    with the distribution.
;
; 3. Neither the name of the copyright holder nor the names of its
;    contributors may be used to endorse or promote products derived
;    from this software without specific prior written permission.
;
; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
; "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
; NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
; FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
; SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
; DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
; DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
; GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
; INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
; WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
; NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
; THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;
; ABSTRACT
; On this particular board, the SS (or Slave Select) signal for the SPI
; is on bit 1 of port B. The function herein sets or resets the state of
; the SS pin.
;
#include <avr/io.h>
;
; void _setss(uchar_t);
;
; Turn on or off the SS pin of the SPI port. Note that when we turn off
; the SS pin by setting it HIGH. As the pin is an open-collector output,
; it takes a bit of time to rise up to Vcc. So to be sure the radio
; actually sees the edge, we wait about 0.5uS for the rise time. Really
; we only need to do this if we're planning on setting it low again
; immediately afterwards, but this is simpler than a lot of tests.
	.global	_setss
	.func	_setss
_setss:
	tst		r24						; SS pin on or off?
	breq	1f
	cbi		_SFR_IO_ADDR(PORTB),1	; Turn on the SS pin (active low)
	ret
1:	sbi		_SFR_IO_ADDR(PORTB),1	; Turn off the SS pin
	nop								; Delay to allow the pin to go high
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	ret
	.endfunc
;
; Fin
