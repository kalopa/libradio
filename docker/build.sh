#!/bin/bash
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
