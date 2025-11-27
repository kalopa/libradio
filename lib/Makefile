#
# Copyright (c) 2020-24, Kalopa Robotics Limited.  All rights reserved.
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
# Yes, this is the Makefile. What can I say? It makes stuff... A lot of
# the real "meat and potatoes" is saved in the avr.mk file which is
# derived from the one in https://github.com/kalopa/libavr.git. It builds
# the main library.
#

DEVICE=	atmega328p

ASRCS=	locore.S ioinit.S radio_irq.S setled.S \
	setss.S testpt.S watchdog.S
CSRCS=	init.c loop.c state.c handle.c command.c \
	rxtx.c packet.c power.c radio.c clock.c \
	wait.c power_mode.c debug.c

include ../avr.mk

all:	$(LIBRADIO)

install: $(LIBRADIO)

clean:
	rm -f $(LIBRADIO) $(OBJS) *.lst tags errs

tags:
	ctags *.[ch]

$(LIBRADIO):	$(OBJS)
	$(AR) cru $(LIBRADIO) $?
	cp $(LIBRADIO) ..

$(OBJS):	internal.h radio_config.h ../libradio.h
