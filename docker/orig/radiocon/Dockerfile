#
# Copyright (c) 2019-25, Kalopa Robotics Limited.  All rights reserved.
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
FROM ruby:3.0.2-alpine
LABEL author="Dermot Tynan <dtynan@kalopa.com>"
LABEL version="1.0"
LABEL description="Docker image to manage low-level radio board."

WORKDIR /app

ENV SERIAL_DEVICE="/dev/ttyUSB0"
ENV SERIAL_SPEED=38400

ENV RABBITMQ_NODE="172.17.0.1"
ENV RABBITMQ_VHOST="/"
ENV RABBITMQ_USER="user"
ENV RABBITMQ_PASS="secret"

RUN apk add --update --no-cache ca-certificates openssl build-base ruby-dev

COPY Gemfile Gemfile.lock /app/
RUN gem install bundler:2.3.5 && bundle install
RUN apk del ruby-dev build-base
COPY daemon.rb /app

CMD ["/app/daemon.rb"]
