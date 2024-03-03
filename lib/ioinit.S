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
; void _ioinit();
	.text
	.section .init4,"ax",@progbits
	.global	_ioinit
	.func	_ioinit
_ioinit:
	ldi		r16,0xee
	ldi		r17,0x02
	out		_SFR_IO_ADDR(PORTB),r17				; PORTB=0000 0010
	out		_SFR_IO_ADDR(DDRB),r16				; DDRB =OOOI OOOI

	ldi		r16,0x37
	ldi		r17,0x00
	out		_SFR_IO_ADDR(PORTC),r17				; PORTC=0000 0000
	out		_SFR_IO_ADDR(DDRC),r16				; DDRC =IIOO IOOO

	ldi		r16,0xfa
	out		_SFR_IO_ADDR(PORTD),r17				; PORTD=0000 0000
	out		_SFR_IO_ADDR(DDRD),r16				; DDRD =OOOO OIOI
	ret
	.endfunc
