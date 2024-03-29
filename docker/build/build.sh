#!/bin/bash
#
# Copyright (c) 2019-24, Kalopa Robotics Limited.  All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# ABSTRACT
# This Docker code wil build the radio images by downloading the latest
# code from Git, for both the libavr library and the libradio library. It
# will then use avr-gcc to compile these into hex images which can be
# programmed on a device.
#
set -e

export AVR=/build/libavr

mkdir /build/output

git clone https://github.com/kalopa/libavr.git
make -C libavr
git clone https://github.com/kalopa/libradio.git

commit_id=$(cd libradio; git rev-parse HEAD | cut -c1-8)
echo "Commit ID: $commit_id"

make -C libradio/lib
make -C libradio/control radiocon.hex
mv libradio/control/radiocon.hex /build/output/radiocon-$commit_id.hex
make -C libradio/monitor firmware.hex
mv libradio/monitor/firmware.hex /build/output/monitor-$commit_id.hex
make -C libradio/examples firmware.hex
mv libradio/examples/firmware.hex /build/output/oiltank-$commit_id.hex
ls -lF /build/output
exit 0
