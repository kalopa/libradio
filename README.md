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
A central controller application (there can be more than one)
is responsible for sending command packets to the client systems
and periodically requesting status to be transmitted back on a
separate channel.
Packets are anywhere from 3 to 46 bytes in length.
The key header components are a destination node ID (or 0x00 for
broadcast), a payload length (including the header) and a
command byte.
See the
[COMMANDS](COMMANDS.md) file for information on the command set.
But note that most commands are application-specific and this
system does not need to understand the command, it just needs
to send the command intact to the specified client (or all
clients, if it is a broadcast).

The main controller will also broadcast a time-of-day update on a
periodic basis, which achieves two things.
Firstly, it means that all devices have the same timestamp.
Secondly, it means that an active device will stay active as
long as it sees radio traffic periodically.

## The State Machine

The library operates a state machine with the following states:

* LIBRADIO\_STATE\_STARTUP
* LIBRADIO\_STATE\_ERROR
* LIBRADIO\_STATE\_COLD
* LIBRADIO\_STATE\_WARM
* LIBRADIO\_STATE\_LISTEN
* LIBRADIO\_STATE\_ACTIVE

Additional states can easily be added, as long as they start with a
value greater than or equal to *NLIBRADIO_STATES*.
When the client code starts up, it immediately goes into a STARTUP
state, and the board LED will remain on.
If any issues arise during startup, such as a radio failure, the
code will drop into an ERROR state.
This is obvious due to the fast flashing of the onboard LED.
Assuming all went well, the client will migrate from a STARTUP
state to a LISTEN state where it will listen for radio traffic.
It will listen for up to two minutes for an activation command to
be received.
Activation commands indicate the operating channel for the client
device, it's channel-unique ID (only valid during the active period),
and four bytes to identify the specific device.
If a valid activation command is received, then the client device
will move to an ACTIVE state until either radio communication is
lost, or the device is sent a deactivation command.

Should the device not receive any radio traffic during the LISTEN
state, it will drop into a COLD sleep.
At this point, any hardware which can be disabled is turned off
and the master clock is slowed down as much as possible to
reduce the number of interrupts waking the CPU from a sleep.
Every hour, the client will switch again to a LISTEN state and
listen for sixty seconds for radio traffic.
If, during a LISTEN state, radio traffic is heard, but no
activation command is received for this device, then it will
drop back to a WARM sleep.
The WARM sleep state is similar to the COLD state except that it will
re-awaken after fifteen minutes rather than an hour, and will listen
for two minutes instead of one minute.

The intent here is that inactive devices will reduce their power
consumption greatly when inactive.
The steps to bring up the network involve first activating the
main controller and telling it which channels it is allowed to
use - this allows for multiple controllers on separate frequencies.
The main controller will broadcast the time of day on any active
channels, as soon as it has been activated.
It will take up to an hour for the client radios on the network
to hear these time broadcasts, and exit hibernation.
If they are not activated, thereafter they will remain in a WARM
state and will check more frequently for activation messages.

## Absolute Client Identification

The library supports an identification system where each device is
pre-configured with four unique bytes.
The first two of these bytes represent the category of client device,
and are user-selectable.
The last two bytes are used to specify the instance of that type
of device.
For example, the reference oil tank measurement client has a major
category ID of 127 and a minor category ID of 1.
These are hard-coded in the client firmware.
The instance number is saved in EEPROM so that there can be up to 65532
oil tank measurement clients on the network.
The values 0x00 and 0xff can not be used as category or instance bytes.
A production line could produce oil tank measurement
devices all with the tuple [127, 1, 0, 1] and then use the EEPROM
updating mechanisms to reprogram each device with a unique instance
number, effectively updating the last two bytes of the tuple.

It is recommended that the first byte, _cat1_ in the code, be used
to specify a major taxonomy of devices and _cat2_ be used to
specify the minor taxonomy.
However, this is not enforced and is merely a suggestion.

## Client Activation

Client activation is not trivial.
All clients are expected to support the main command set, which
comprises commands in the range 0x00 -> 0x0f.
To activate a client, a broadcast message is sent on channel 0.
This message has a payload of six bytes, which are defined as follows:

1. Client channel ID
2. Client node ID
3. Selected client category byte #1
4. Selected client category byte #2
5. Selected client instance byte #1
3. Selected client instance byte #2

The channel ID tells the client to move away from channel 0 for future
command reception, as channel 0 is usually used for low-volume traffic
announcing activation messages only.
However, this is a convention and channel 0 can actually be used
for all communications, if desired.

The client node ID is used by the main controller to identify this
particular station as long as it is active.
Node IDs are assigned and re-used on an as-needed basis.
No assumptions should be made about the client node ID other than the
fact that it will never be set to zero.
Clients which have just started up will only listen for broadcast
messages until they are assigned a node ID.

The four bytes of node selection are defined in the previous section.

This activation message should be sent, several times over the course
of a few minutes (and indeed up to fifteen minutes in the case of
a device in a WARM state).
On the assigned radio channel, a *RADIO_CMD_STATUS* command is
transmitted on a periodic basis by default, so that the
active stations will report their status.
The status block of a client station is application-specific.
The intent is that a status command will be received on a separate
channel, and forwarded to any down-stream software which might
be interested in that device.
As the entire system is open-loop and asynchronous,
there is no definitive way to know if a device has
received the activation command and is now active and online.
The best approach is to repeat the activation message every
minute, until such time as a status response is received.
due to the cold sleep mechanism, this could take up to an hour.
In practice, all devices will move to a warm sleep once the
radio has been heard transmitting.
In that case, it should take less than fifteen minutes
for the activation to be heard, and another few minutes
for the station to report its status on foot of a request
on the assigned channel.

Eventually the requested station will appear on the network,
or the various down-stream systems will give up.

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
