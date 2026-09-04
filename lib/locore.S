;
; Copyright (c) 2019-25, Kalopa Robotics Limited.  All rights
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
	jmp		t2int				; Timer 2 Compare Match B
	jmp		empty				; Timer 2 Overflow
	jmp		empty				; Timer 1 Capture Event
	jmp		t1int				; Timer 1 Compare Match A
	jmp		empty				; Timer 1 Compare Match B
	jmp		empty				; Timer 1 Counter Overflow
	jmp		t0int				; Timer 0 Compare Match A
	jmp		empty				; Timer 0 Compare Match B
	jmp		empty				; Timer 0 Counter Overflow
	jmp		spi_irq				; SPI Serial Transfer Complete
	jmp		sio_in				; USART Rx Complete
	jmp		sio_out				; USART Data Register Empty
	jmp		empty				; USART Tx Complete
	jmp		empty				; ADC Conversion Complete
	jmp		empty				; EEPROM Ready
	jmp		empty				; Analog Comparator
	jmp		empty				; Two Wire Interface
	jmp		empty				; Store Program Memory Ready
;
empty:
	reti						; Return from interrupt
	jmp		_ioinit				; Need to pull this in, now.
	.endfunc
;
; TIMER0 interrupt. This runs at 125Hz (every 8ms). It calls the C code
; to update the date and time, and flash the LED (as appropriate).
	.section .init6,"ax",@progbits
	.global		t0int
	.func		t0int
t0int:
	push	r24					; Save the status register
	in		r24,_SFR_IO_ADDR(SREG)
	push	r24
;
; Call the C code with our main clock interrupt service routine
	push	r18					; Save the temporary regs
	push	r25
#ifdef __AVR_HAVE_JMP_CALL__
	call	clocktimer			; Call the main clock tick (call)
#else
	rcall	clocktimer			; Call the main clock tick (rcall)
#endif
	pop		r25
	pop		r18
;
; We're done.
t0ret:
	pop		r24
	out		_SFR_IO_ADDR(SREG),r24
	pop		r24
	reti						; Return from interrupt
	.endfunc
;
; TIMER1 interrupt. This is running at 10kHz and is used to determine
; radio slot times. Essentially save the current OCR1A value and set it
; to the next slot time.
	.section .init5,"ax",@progbits
	.global		t1int
	.func		t1int
t1int:
	push	r24					; Save the status register
	in		r24,_SFR_IO_ADDR(SREG)
	push	r24
	push	r25					; Save the working regs
;
; Get the current value of TCNT1 and save it into _slot_prev.
	lds		r24,TCNT1
	lds		r25,TCNT1+1
	sts		_slot_prev,r24
	sts		_slot_prev+1,r25
;
; Now set OCR1A to the next slot time.
	lds		r24,_slot_next
	lds		r25,_slot_next+1
	sts		OCR1A+1,r25
	sts		OCR1A,r24
;
; We're done.
	pop		r25
	rjmp		t0ret
	.endfunc
;
; TIMER2 interrupt. Handle the servo motor output.
	.section .init6,"ax",@progbits
	.global		t2int
	.func		t2int
t2int:
	push	r24					; Save the status register
	in		r24,_SFR_IO_ADDR(SREG)
	push	r24
;
; We're done.
	rjmp		t0ret
	.endfunc
;
; Radio module interrupt.
	.text
	.section .init4,"ax",@progbits
	.global	radio_irq
	.func	radio_irq
radio_irq:
	push	r24						; Save the status register
	in		r24,_SFR_IO_ADDR(SREG)
	push	r24
	push	r25						; Save the working regs
;
	ldi		r24,1
	out		_SFR_IO_ADDR(EIFR),r24	; Clear the interrupt
	lds		r24,_irq_fired
	inc		r24
	sts		_irq_fired,r24
	cbi		_SFR_IO_ADDR(EIMSK),0	; Disable the interrupt too (for now)
;
	pop		r25						; Restore the status register
	rjmp		t0ret
	.endfunc
;
; Define the heap_end for malloc()
	.section .init0,"ax",@progbits
	.weak	__heap_end
	.set	__heap_end, 0
;
; Fin
