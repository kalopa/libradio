;
; Copyright (c) 2019-24, Kalopa Robotics Limited.  All rights
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
;
#include <avr/io.h>
;
; Radio module interrupt.
	.text
	.section .init4,"ax",@progbits
	.global	radio_irq
	.func	radio_irq
radio_irq:
	push	r24					; Save the status register
	in		r24,_SFR_IO_ADDR(SREG)
	push	r24
	push	r25					; Save the working regs
;
	ldi		r24,1
	out		_SFR_IO_ADDR(EIFR),r24		; Clear the interrupt
	lds		r24,_irq_fired
	inc		r24
	sts		_irq_fired,r24
	cbi		_SFR_IO_ADDR(EIMSK),0		; Disable the interrupt too (for now)
;
	pop		r25					; Restore the status register
	pop		r24
	out		_SFR_IO_ADDR(SREG),r24
	pop		r24
	reti						; Return from interrupt
	.endfunc
