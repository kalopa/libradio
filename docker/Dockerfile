FROM ubuntu:jammy
LABEL author="Dermot Tynan <dtynan@kalopa.com>"
LABEL version="1.0"
LABEL description="Docker image to build libradio hex files."

RUN apt-get update --fix-missing
RUN apt-get install -y gcc-avr binutils-avr avr-libc make git

WORKDIR /build

COPY build.sh /build

CMD ["/build/build.sh"]
