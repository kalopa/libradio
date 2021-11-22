;
; Copyright (c) 2019-21, Kalopa Robotics Limited.  All rights
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
; This AVR assembly code contains all the necessary startup code for
; a standard Kalopa AVR radio board. It has the main startup code,
; the IRQ vector table and some of the main IRQ functions.  The radio
; IRQ and the SPI IRQ are reserved for future use.
;
; The clock IRQ just saves some registers and calls the C code. The
; ioinit() function is used to configure the AVR I/O ports and do any
; special start-up in the I/O system (stuff that might be specific to
; this device).
;
; NOTE: The other functions called (such as _reset, sio_in and sio_out)
; are in the Kalopa libavr library - https://github.com/kalopa/libavr.
;
#include <avr/io.h>
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
	in		r24,_SFR_IO_ADDR(SREG)
	push	r24
	push	r25					; Save the working regs
;
	push	r18					; Save the temporary regs
	push	r19
	push	r26
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
	out		_SFR_IO_ADDR(SREG),r24
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
	out		_SFR_IO_ADDR(PORTB),r17
	out		_SFR_IO_ADDR(DDRB),r16

	ldi		r16,0x37			; DDRC=IIOO IOOO
	ldi		r17,0x00			; PORTC=0000 0000
	out		_SFR_IO_ADDR(PORTC),r17
	out		_SFR_IO_ADDR(DDRC),r16

	ldi		r16,0xfa			; DDRD=OOOO OIOI
	out		_SFR_IO_ADDR(PORTD),r1			; PORTD=0000 0000
	out		_SFR_IO_ADDR(DDRD),r16
	ret
	.endfunc
;
; Fin
