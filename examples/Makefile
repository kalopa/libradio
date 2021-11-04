#
# Copyright (c) 2020-21, Kalopa Robotics Limited.  All rights reserved.
# Unpublished rights reserved under the copyright laws  of the Republic
# of Ireland.
#
# The software contained herein is proprietary to and embodies the
# confidential technology of Kalopa Robotics Limited.  Possession,
# use, duplication or dissemination of the software and media is
# authorized only pursuant to a valid written license from Kalopa
# Robotics Limited.
#
# RESTRICTED RIGHTS LEGEND   Use, duplication, or disclosure by
# the U.S.  Government is subject to restrictions as set forth in
# Subparagraph (c)(1)(ii) of DFARS 252.227-7013, or in FAR 52.227-19,
# as applicable.
#
# THIS SOFTWARE IS PROVIDED BY KALOPA ROBOTICS LIMITED "AS IS" AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL KALOPA ROBOTICS LIMITED
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
# OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
# OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
# BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
#
# ABSTRACT
# This is the Makefile to build the firmware for the example oil tank
# measurement device.
#
DEVICE=	atmega328p

ASRCS=	locore.S
CSRCS=	main.c
BIN=	oiltank

# https://eleccelerator.com/fusecalc/fusecalc.php
LFUSE=0xef
HFUSE=0xdf
EFUSE=0xff

include ../avr.mk

$(BIN):	$(OBJS) $(LIBRADIO)
		$(CC) -o $(BIN) $(LDFLAGS) $(OBJS) $(LIBS)

$(OBJS): ../libradio.h
