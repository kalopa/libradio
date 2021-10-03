;
; Copyright (c) 2019-21, Kalopa Robotics Limited.  All rights reserved.
; Unpublished rights reserved under the copyright laws  of the Republic
; of Ireland.
;
; The software contained herein is proprietary to and embodies the
; confidential technology of Kalopa Robotics Limited.  Possession,
; use, duplication or dissemination of the software and media is
; authorized only pursuant to a valid written license from Kalopa
; Robotics Limited.
;
; RESTRICTED RIGHTS LEGEND   Use, duplication, or disclosure by
; the U.S.  Government is subject to restrictions as set forth in
; Subparagraph (c)(1)(ii) of DFARS 252.227-7013, or in FAR 52.227-19,
; as applicable.
;
; THIS SOFTWARE IS PROVIDED BY KALOPA ROBOTICS LIMITED "AS IS" AND ANY
; EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
; IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
; PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL KALOPA ROBOTICS LIMITED
; BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
; OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
; OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
; BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
; WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
; OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
;
; ABSTRACT
; This AVR assembly code contains all the necessary startup code for our
; oil tank measurement device. The only interrupts which are (currently)
; used are for serial I/O and for the clock/timer (OC1A), which is used
; for delays and real-time clock management. The radio IRQ and the SPI
; IRQ are reserved for future use.
;
; The clock IRQ just saves some registers and calls the C code. The main
; startup/reset code follows. It initializes the stack, clears memory,
; sets up BSS and calls main(). Finally, the setled function is used to
; turn on and off an onboard LED so we can see what the code is doing.
;
; NOTE: The sio_in and sio_out interrupt service routines are in the
; Kalopa libavr library.
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
irq_done:
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
; void _ioinit();
;
; Configure the I/O pins as appropriate (called from the reset code).
	.text
	.section .init4,"ax",@progbits
	.global	_ioinit
	.func	_ioinit
_ioinit:
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
	ret
	.endfunc
;
; void _setled(uchar_t);
;
; Turn on or off the LED.
	.global	_setled
	.func	_setled
_setled:
	tst		r24					; LED on or off?
	breq	sl1
	cbi		PORTD,3				; Turn on the LED
	ret
sl1:
	sbi		PORTD,3				; Turn off the LED
	ret
	.endfunc
;
; Some useful (low level) variables...
	.weak	__heap_end
	.set	__heap_end,0
;
; Fin
