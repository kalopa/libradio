;
; Copyright (c) 2020-21, Kalopa Robotics Limited.  All rights reserved.
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
;
.include "atmega328p.inc"
;
; void _setss(uchar_t);
;
; Turn on or off the SS pin of the SPI port. Note that when we 
	.global	_setss
	.func	_setss
_setss:
	tst		r24					; SS pin on or off?
	breq	ss1
	cbi		PORTB,1				; Turn on the SS pin (active low)
	ret
ss1:
	sbi		PORTB,1				; Turn off the SS pin
	nop							; Delay to allow the pin to go high
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
