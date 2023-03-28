#
# Copyright (c) 2020-21, Kalopa Robotics Limited.  All rights reserved.
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
# Include file for AVR Makefiles... This is where all the actual build
# complexity lives.
#
OS?=$(shell uname)

ifeq (${OS}, Darwin)
	BINDIR=/usr/local/bin
else
ifeq (${OS}, FreeBSD)
	BINDIR=/usr/local/bin
else
	BINDIR=/usr/bin
endif
endif

AVR?=../libavr
STDPROM=$(AVR)/stdprom.x

DEVICE?=atmega328p
PROG?=usbtiny
OBJS?=$(ASRCS:.S=.o) $(CSRCS:.c=.o)

AR=$(BINDIR)/avr-ar
AS=$(BINDIR)/avr-as
CC=$(BINDIR)/avr-gcc
C++=$(BINDIR)/avr-c++
G++=$(BINDIR)/avr-g++
GCC=$(BINDIR)/avr-gcc
LD=$(BINDIR)/avr-ld
NM=$(BINDIR)/avr-nm
OBJDUMP=$(BINDIR)/avr-objdump
RANLIB=$(BINDIR)/avr-ranlib
STRIP=$(BINDIR)/avr-strip

LIBRADIO=../lib/libradio.a
FIRMWARE?=firmware.hex

# https://eleccelerator.com/fusecalc/fusecalc.php
LFUSE?=0xef
HFUSE?=0xdf
EFUSE?=0xfc

ASFLAGS= -mmcu=$(DEVICE) -I$(AVR)
CFLAGS=	-Wall -O2 -mmcu=$(DEVICE) -I$(AVR) -I..
LDFLAGS=-nostartfiles -u __vectors -mmcu=$(DEVICE) -L$(AVR) -Wl,--section-start=.bstrap0=0x7e00
LIBS=	-L../lib -lradio -lavr.$(DEVICE) 

all:	$(BIN)

clean:
	rm -f $(BIN) $(OBJS) $(FIRMWARE) $(EXTRA_CLEAN) *.lst srclist.ps srclist.pdf errs

program: $(FIRMWARE)
	sudo avrdude -p $(DEVICE) -c $(PROG) -U flash:w:$(FIRMWARE):i
	rm $(FIRMWARE)

fuses:
	sudo avrdude -p $(DEVICE) -c $(PROG) -U lfuse:w:$(LFUSE):m -U hfuse:w:$(HFUSE):m -U efuse:w:$(EFUSE):m

erase:
	sudo avrdude -p $(DEVICE) -c $(PROG) -e

docs:	srclist.pdf

$(FIRMWARE): $(BIN)
	avr-objcopy -O ihex $(BIN) $(FIRMWARE)

srclist.ps: $(CSRCS) $(ASRCS)
	c2ps $(CSRCS) $(ASRCS) > srclist.ps

srclist.pdf: srclist.ps
	ps2pdf srclist.ps srclist.pdf

%.o:	%.S
	$(CC) -E -mmcu=$(DEVICE) $< > temp.s
	$(AS) $(ASFLAGS) -o $(<:.S=.o) temp.s
	rm temp.s

%.lst:	%.c
	$(CC) $(CFLAGS) -Wa,-adhlns=$(<:%.c=%.lst) -c -o $(<:%.c=%.o) $<

%.lst:	%.S
	$(AS) $(ASFLAGS) -adhlns=$(<:%.S=%.lst) -o $(<:%.S=%.o) $<
