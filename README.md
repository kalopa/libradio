# libradio - an AVR library for 434MHz communications

This repo has the source code (and Makefile) to build a master/slave
radio communications system based on the Si4463 radio.
The code is designed to work with the Kalopa Robotics microcontroller
board which features an Atmel ATMega328P CPU and a Si4463 radio
module.
The Si4463 is quite a complicated radio and requires a lot of setup
and hand-holding.
But it is an amazing transceiver with a wide range of functions and
abilities.

The entire system uses multiple channels within the 434MHz frequency
range.

## Signalling

See the [SIGNALLING](SIGNALLING.md) file for more information.

## Si4463 Details

- Chip Mask Revision: 11
- Part: 4463
- Part Build: 0
- ID: 15
- Customer ID: 0
- RevInt: f
- Patch: 0
- Func: 1

Digikey shows five chips, of which one (B0B) is marked as obsolete.
These are:

- B0B-FM
- B1B-FM
- B1B-FMR
- C2A-GM
- C2A-GMR

The modules I'm using are marked 44631B.
