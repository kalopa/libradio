# libradio - an AVR library for 434MHz communications

This repo has the source code (and Makefile) to build a master/slave
radio communications system based on the Si4463 radio.
The code is designed to work with the Kalopa Robotics microcontroller
board which features an Atmel ATMega328P CPU and a Si4463 radio
module.
However, any Arduino and Si4463 combination will work.
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
Packets are anywhere from 6 to 22 bytes in length.
The key header components are a
two byte time stamp, a destination node ID (or 0x00 for
broadcast), a data length (not including the header), a command byte
and a checksum (currently unused).
This is followed by zero or more data bytes.
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

The onboard LED will flash in a certain sequence depending on what state
the system is currently in.
Using software to flash the LED on and off means that it is a sure
indication that the software is actually doing something.
What's more, the different sequences offer more information than just *ON*
or *OFF*.

| State | LED Sequence |
| :---: | :---: |
| LIBRADIO\_STATE\_STARTUP | Solid "ON" |
| LIBRADIO\_STATE\_ERROR | Fast flashing on/off |
| LIBRADIO\_STATE\_COLD | 1.6s flash every hour |
| LIBRADIO\_STATE\_WARM | 3.2s flash every fifteen minutes |
| LIBRADIO\_STATE\_LISTEN | On/off flashing sequence |
| LIBRADIO\_STATE\_ACTIVE | "Heartbeat" sequence |

For user-added states, the LED will just indicate an ACTIVE state.

## Radio Commands

Commands are sent by a main controller to one or more clients.
No response is anticipated or expected, except as noted below.
The main controller can send command packets to client devices
in a round-robin fashion, or favour certain devices over others.
It can use a single channel (such as channel 0) for all
communication, or it may interleave the channels to put high
volume traffic on a separate network.

It may also either use the channel in half-duplex, send/receive
mode, or dedicate a separate channel for receiving status
responses from clients.
Client status responses should not be tied to status requests
as the system is asynchronous.
In theory, a status response could be sent unilaterally,
although this is not advised.

Every command packet has a minimum of six bytes, followed
by a data payload of up to 22 bytes.

1. ms\_ticks (low byte)
2. ms\_ticks (high byte)
3. Destination node ID
4. Packet length
5. Command
6. Checksum

The ms\_ticks value represents the number of tens of milliseconds of
absolute date/time and is set by default by the transmit code
and retrieved automatically by the receive code.
If the time of day counter is running (see the **Time and Date**
section below), then this value will automatically increase after
every clock interrupt.
There is a real requirement for the main controller and all client
applications to have a synchronized clock, and this is how it
gets updated.

The destination node ID will be used by the main controller to identify
this particular client station as long as it is active.
Node IDs are assigned and re-used on an as-needed basis.
No assumptions should be made about the client node ID other than the
fact that it will never be set to zero.
A node ID of 0x00 is a broadcast address and will be received by
all clients on that broadcast channel.
Clients which have just started up will only listen for broadcast
messages until they are assigned a node ID.

The command set has up to sixteen reserved commands which are used
for basic state machine and date/time manipulation.
The main controller and the client library do not understand commands
above 0xf and these are passed to the client application for processing.
Application-specific commands start at 0x10 up to 0xff.
Similarly, status returned by a client application is considered opaque
to the main controller and the client library.
It should be processed and interpreted by an upstream system.

### Command: RADIO\_CMD\_NOOP

This command does nothing.
It could be used to keep devices in a WARM sleep,
but generally the SET\_TIME and SET\_DATE commands are used for that
purpose.

### Command: RADIO\_CMD\_FIRMWARE

This command is used to re-flash the client firmware and is reserved
for later use.

### Command: RADIO\_CMD\_STATUS

The main controller has requested the client device return its status.
The command takes three arguments:

1. The response channel - the radio channel the client
should use to transmit its status.
2. The node ID of the receiving station on that channel.
3. The status type - the application-specific status information requested
by the controller.

Note that status type 0x00 should always be supported.
The main controller will request status type 0x00 from
all active devices on a frequent basis and without
any upstream prompting.
The status could indicate standard fare such as battery voltage
and time of day.
Again, it is immaterial as the status response is application-specific
and not interpreted by the main controller.

### Command: RADIO\_CMD\_ACTIVATE

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

The client node ID will be used by the main controller to identify this
particular station as long as it remains active.
The four bytes of node selection are defined in the previous section.

### Command: RADIO\_CMD\_DEACTIVATE

The deactivate command simply forces the client device to deactivate,
releasing its node ID and moving to a WARM state.
The node ID can be re-used, after some form of quarantine period.
The command takes no arguments.

### Command: RADIO\_CMD\_SET\_TIME

This command is used to synchronize the tens\_of\_minutes clock on the
main controller with one or more of the clients.
Generally it is sent as a broadcast to all clients, but this is not
essential.
The actual millisecond/second time is always sent in every packet and
received by the client for internal use.

The time and date functionality of the system is a little unorthodox.
Read the section on **Time and Date** below for more information.

The command has a data length of one byte which is the time of
day in the range of 0 -> 143.
It is also possible to disable clock functionality across the entire
system by broadcasting a SET\_TIME command with a value of 0xff.
However, this doesn't appear to be entirely useful.

### Command: RADIO\_CMD\_SET\_DATE

The date is not used or incremented by the library or by the main
controller.
However, the value can be transmitted from time to time
during the day in case any client application needs the
information.

### Command: RADIO\_CMD\_READ\_EEPROM

This command will read up to sixteen (16) bytes of EEPROM data and send
it in a response packet.
It takes five arguments:

1. The response channel - the radio channel the client
should use to transmit its status.
2. The node ID of the receiving station on that channel (not the client).
3. The number of bytes to read.
4. The lower 8 bits of the EEPROM address for reading.
5. The upper 8 bits of the EEPROM address for reading.

Similar to the CMD\_STATUS above, the data is returned on an out-of-band
channel.

### Command: RADIO\_CMD\_WRITE\_EEPROM

This command will write up to sixteen (16) bytes of EEPROM data.
It takes 3 arguments and at least one byte of EEPROM data.

1. The number of bytes to write (minimum is 1).
2. The lower 8 bits of the EEPROM address for writing.
3. The upper 8 bits of the EEPROM address for writing.

The remaining bytes in the packet are for the 1 to 16 bytes of data.

As with all the radio commands, there is no confirmation that the
operation has been received, much less completed.
Use the RADIO\_CMD\_READ\_EEPROM to verify EEPROM contents are
as anticipated.

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

## Time and Date

Three variables are used to record the date and time, to the nearest 10ms.
The main time loop is **ms_ticks** and this is used to count clock
ticks with a frequency of 10ms per tick.
As it is an unsigned 16 bit number, it will roll over every ten minutes.
In fact, it will never get larger than 59,999.
the next variable is the time of day in increments of ten minutes.
This can range from 0 through to 143 over the course of a day.
It can, under certain circumstances, go above 143.
For events planned up to 18:19PM on the following day, the time of
day value can be anywhere from 144 through to 254.
Generally though, this should be avoided.
The value of 0xff (255) is reserved for the special case where
the time of day is not known.
When the **tens_of_minutes** value is set to 0xff, then all
time of day calculations are stopped.
Also, the interrupt service routine which manages the time of
day will also stop incrementing the **ms\_ticks** value as
well.

The third and final variable is the **date** variable which is
sixteen bits and application-specific.
The value is given to the main controller and is rebroadcast
throughout the network.
As it is not used by the library nore the controller, it can
mean anything.
In fact, it can be ignored, if desired.
An advised (but not enforced) convention might be to set this
to the number of days since January 1st, 2000.
Note that none of the code herein will increment or
set this value, so it is up to the higher-level applications
to set the value and for the low-level client applications to
make use of it.

There is an interesting artifact with the way the tens of minutes
functionality works in that in the absence of an update after
midnight, the value will continue from 144 onwards.
It will not wrap around.
This means that if the system has not issued a time command
in the period from midnight until 18:20 that day, then
the value will automatically increment to 0xff and stop
all clock mechanisms.
Ideally a time packet would be transmitted on a regular basis
by the main controller, so this should never occur.
Also, it is more likely that the state machine will
move the client to a WARM or COLD state if no radio traffic
is received, and both of those states will also stop
the time of day clock.

## Radio Details

The Si4463 radio chip is a complex device.
Aliexpress sells them as a simple board with some signal conditioning
circuits and a crystal oscillator.

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
