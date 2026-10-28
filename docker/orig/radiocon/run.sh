#!/bin/sh
set -ex

docker build -t radiocon:latest .
docker run -it \
    --device=/dev/ttyUSB0 \
    -e RABBITMQ_NODE="172.17.0.1" \
    -e RABBITMQ_VHOST="/radiocon" \
    -e RABBITMQ_USER="radiocon" \
    -e RABBITMQ_PASS="FooBar123" \
    -e SERIAL_DEVICE="/dev/ttyUSB0" \
    -e SERIAL_SPEED=38400 \
    radiocon:latest
exit 0
