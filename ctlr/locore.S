;
; Copyright (c) 2020-21, Kalopa Robotics Limited.  All rights
; reserved.
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions
; are met:
;
; 1. Redistributions of source code must retain the above copyright
;    notice, this list of conditions and the following disclaimer.
;
; 2. Redistributions in binary form must reproduce the above copyright
;    notice, this list of conditions and the following disclaimer
;    in the documentation and/or other materials provided with the
;    distribution.
;
; 3. Neither the name of the copyright holder nor the names of its
;    contributors may be used to endorse or promote products derived
;    from this software without specific prior written permission.
;
; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
; "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
; LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
; FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
; COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
; INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
; BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
; CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
; LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
; IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
; THE POSSIBILITY OF SUCH DAMAGE.
;
; ABSTRACT
;
.include "atmega328p.inc"
;
; Interrupt Vector table.
	.arch   atmega328p
	.section .vectors,"ax",@progbits
	.global	__vectors
	.func	__vectors
__vectors:
	jmp		_reset				; Main reset
	jmp		radio_irq			; External Interrupt 0
	jmp		empty				; External Interrupt 1
	jmp		empty				; Pin Change Interrupt Request 0
	jmp		empty				; Pin Change Interrupt Request 1
	jmp		empty				; Pin Change Interrupt Request 2
	jmp		empty				; Watchdog Timeout Interrupt
	jmp		empty				; Timer 2 Compare Match A
	jmp		empty				; Timer 2 Compare Match B
	jmp		empty				; Timer 2 Overflow
	jmp		empty				; Timer 1 Capture Event
	jmp		clock_irq			; Timer 1 Compare Match A
	jmp		empty				; Timer 1 Compare Match B
	jmp		empty				; Timer 1 Counter Overflow
	jmp		empty				; Timer 0 Compare Match A
	jmp		empty				; Timer 0 Compare Match B
	jmp		empty				; Timer 0 Counter Overflow
	jmp		spi_irq				; SPI Serial Transfer Complete
	jmp		sio_in	 			; USART Rx Complete
	jmp		sio_out				; USART Data Register Empty
	jmp		empty				; USART Tx Complete
	jmp		empty				; ADC Conversion Complete
	jmp		empty				; EEPROM Ready
	jmp		empty				; Analog Comparator
	jmp		empty				; Two Wire Interface
	jmp		empty				; Store Program Memory Ready
;
empty:
	reti	 					; Return from interrupt
;
; Handle a clock tick.
clock_irq:
	push	r24					; Save the status register
	in		r24,SREG
	push	r24
	push	r25					; Save the working regs
;
	push	r18
	push	r19
	push	r26					; Save the temporary regs
	push	r27					; Save the temporary regs
	rcall	clocktick			; Call the main clock tick
	pop		r27					; Restore the registers
	pop		r26
	pop		r19
	pop		r18
;
	pop		r25					; Restore the status register
	pop		r24
	out		SREG,r24
	pop		r24
	reti						; Return from interrupt
;
; Radio module interrupt.
radio_irq:
	reti
;
; Serial Peripheral Interface (SPI) interrupt.
spi_irq:
	reti
	.endfunc
;
; void _reset();
;
; To begin, initialize the stack frame, configure the timers and
; I/O pins, as well as the serial port (if fitted), enable interrupts,
; and start working.  The main loop here does several things.  The
; key task is to measure the windmill rotational velocity and try
; to keep it within bounds by altering the pulse width of the output.
	.text
	.section .init0,"ax",@progbits
	.global	_reset
	.func	_reset
_reset:
	.section .init2,"ax",@progbits
	cli
	clr		r1
	out		SREG,r1
	ldi		r16,lo8(RAMEND)		; Set up the stack pointer
	ldi		r17,hi8(RAMEND)
	out		SPH,r17
	out		SPL,r16

	ldi		r16,0xee			; DDRB=OOOI OOOI
	ldi		r17,0x02			; PORTB=0000 0010
	out		PORTB,r17
	out		DDRB,r16

	ldi		r16,0x37			; DDRC=IIOO IOOO
	ldi		r17,0x00			; PORTC=0000 0000
	out		PORTC,r17
	out		DDRC,r16

	ldi		r16,0xfa			; DDRD=OOOO OIOI
	out		PORTD,r1			; PORTD=0000 0000
	out		DDRD,r16
;
; Initialize BSS and data memory.
	ldi		r17,hi8(__data_end)		; End of data block
	ldi		r26,lo8(__data_start)	; X = start of data
	ldi		r27,hi8(__data_start)
	ldi		r30,lo8(__data_load_start)	; Z = program memory
	ldi		r31,hi8(__data_load_start)
	rjmp	bssl2
bssl1:
	lpm							; Fetch a byte from prog mem
	adiw	r30,1				; Increment Z
	st		x+,r0				; Save in [X]
bssl2:
	cpi		r26,lo8(__data_end) ; We done yet?
	cpc		r27,r17
	brne	bssl1				; No, copy some more

	ldi		r17,hi8(__bss_end)	; Point to end of memory
	rjmp	bssl4
bssl3:
	st		x+,r1				; Clear memory
bssl4:
	cpi		r26,lo8(__bss_end)	; We done yet
	cpc		r27,r17
	brne	bssl3				; No, keep trucking
;
; We're ready for prime time!  Turn on interrupts and call the C code.
	call	main 				; Call the main routine
	rjmp	_reset
	.endfunc
;
; void sio_in();
;
; Serial I/O input interrupt routine (RX).
	.section .init6,"ax",@progbits
	.global	sio_in
	.func	sio_in
sio_in:
	push	r24					; Save the status register
	in		r24,SREG
	push	r24
	push	r25
	push	r26
	push	r27
;
	lds		r26,UDR0			; Pull the character
	lds		r24,ihead			; Get head & tail
	lds		r25,itail
	inc		r24
	andi	r24,31
	cp		r24,r25				; Buffer full?
	breq	siol1				; No choice but to drop the character
;
	lds		r24,ihead
	mov		r25,r26
	ldi		r26,lo8(iring)
	ldi		r27,hi8(iring)
	add		r26,r24				; Add the head pointer
	adc		r27,r1				; R1 is always zero!!!
	st		X,r25				; Save the character
	inc		r24
	andi	r24,31
	sts		ihead,r24			; Save the new head pointer
;
	rjmp	siol1				; We're done
	.endfunc
;
; void sio_out();
;
; Serial I/O output interrupt routine (TX).
	.global	sio_out
	.func	sio_out
sio_out:
	push	r24					; Save the status register
	in		r24,SREG
	push	r24
	push	r25
	push	r26
	push	r27
;
	ldi		r26,lo8(oring)		; Get the base ring buffer addr
	ldi		r27,hi8(oring)
	lds		r24,otail			; Add the tail pointer
	add		r26,r24
	adc		r27,r1				; R1 is always zero!!!
	ld		r25,X				; Get the character in the ring
	sts		UDR0,r25			; Transmit it
;
	inc		r24					; Increment the tail pointer
	andi	r24,31
	sts		otail,r24
	lds		r25,ohead			; Get the head pointer
	cp		r24,r25				; Buffer empty?
	brne	siol1				; No, continue
	rcall	_sio_txintoff
;
; Common return routine from interrupts.
siol1:
	pop		r27
	pop		r26
	pop		r25					; Restore the status register
	pop		r24
	out		SREG,r24
	pop		r24
	reti						; Return from interrupt
	.endfunc
;
; void _sio_rxinton();
;
; Enable transmission interrupts.
	.global _sio_rxinton
	.func   _sio_rxinton
_sio_rxinton:
	lds		r24,UCSR0B			; Enable RX ints
	ori		r24,0x80
	sts		UCSR0B,r24
	ret
	.endfunc
;
; void _sio_rxintoff();
;
; Disable transmission interrupts.
	.global	_sio_rxintoff
	.func	_sio_rxintoff
_sio_rxintoff:
	lds		r24,UCSR0B			; Disable RX ints
	andi	r24,0x7f
	sts		UCSR0B,r24
	ret
	.endfunc
;
; void _sio_txinton();
;
; Enable transmission interrupts.
	.global _sio_txinton
	.func   _sio_txinton
_sio_txinton:
	lds		r24,UCSR0B			; Enable TX ints
	ori		r24,0x20
	sts		UCSR0B,r24
	ret
	.endfunc
;
; void _sio_txintoff();
;
; Disable transmission interrupts.
	.global _sio_txintoff
	.func   _sio_txintoff
_sio_txintoff:
	lds		r24,UCSR0B			; Disable TX ints
	andi	r24,0xdf
	sts		UCSR0B,r24
	ret
	.endfunc
;
; Some useful (low level) variables...
	.weak	__heap_end
	.set	__heap_end,0
;
; Fin
