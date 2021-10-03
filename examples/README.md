# Example client code for libradio

This is an example client application for libradio.
It measures the amount of oil in a tank, using an ultrasonic module such
as the HR-C04.
The idea is to produce the simplest possible application which can work
with the library to perform a useful function.

## Operation

Libradio devices boot up in a REBOOT state.
From there, they go to a SLEEP mode, after some basic initialization.
The idea is that the device will consume very little power (if anything),
wake up every few minutes, enable the radio receiver, and listen for
broadcasts on radio channel 0 (the channel reserved for init messages).
To initialize the device, the main transmitter (code in _../ctlr_) will
broadcast a message with the unique identification string of the client
device, along with an assigned channel and identification number.
The client will then move to an ACTIVE state, switch radio reception
to the assigned channel, and listen for messages with the given
identification number, or a broadcast number.

There are a few built-in command functions in libradio, for things like
initialization and shutting down, but the bulk of the commands are
application-specific.
In this case, the device has no built-in commands.
It responds to requests for status or requests to shut down.

## Hardware

All of the libradio interactions are based around the Si4463 radio chip.
The device runs on a frequency of 434MHz and interfaces to an Atmel
chip using the SPI signals, thus leaving the I2C and RS232 interfaces
free for other applications.
This code was designed to run on the Kalopa AVR-Radio board, but can
be modified to work with just about any Atmel AVR chip, and Si4463
board.
