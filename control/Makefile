#
# Copyright (c) 2020-23, Kalopa Robotics Limited.  All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer
#    in the documentation and/or other materials provided with the
#    distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
# IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.
#
# ABSTRACT
# This is the Makefile for the main controller. Most of the heavy lifting
# (again) is done in avr.mk.
#
DEVICE=	atmega328p

ASRCS=
CSRCS=	main.c init.c command.c enqueue.c transmit.c input.c
BIN=	radiocon
FIRMWARE=radiocon.hex

SERIAL_DEVICE=/dev/ttyUSB0:38400

include ../avr.mk

CFLAGS+=	-I../lib

kprog:	$(FIRMWARE)
	sudo kprog -v -d $(SERIAL_DEVICE) $(FIRMWARE)
	rm $(FIRMWARE)

tags:
	ctags *.[ch] ../lib/*.[ch]

$(BIN):	$(OBJS) $(LIBRADIO)
	$(CC) -o $(BIN) $(LDFLAGS) $(OBJS) $(LIBS)

$(OBJS): ../libradio.h ../lib/internal.h control.h
