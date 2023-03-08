# Radio Commands

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

1. ms\_ticks (high byte)
2. ms\_ticks (low byte)
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

## Command: RADIO\_CMD\_NOOP

This command does nothing.
It could be used to keep devices in a WARM sleep,
but generally the SET\_TIME and SET\_DATE commands are used for that
purpose.

Payload: 0 bytes

## Command: RADIO\_CMD\_FIRMWARE

This command is used to re-flash the client firmware and is reserved
for later use.

The sender can transmit two bytes of address and up to twenty
bytes of program data which will force the receiver into the bootstrap
code (if it isn't already), program the given bytes,
and listen for subsequent packets of firmware data.

Payload: 3-22 bytes: ah al nn [nn nn ...]

Two bytes of address (*ah/al*) and up to twenty bytes of firmware data
(*nn*) to be programmed at that address.

Not currently implemented, unfortunately.

## Command: RADIO\_CMD\_STATUS

The main controller has requested the client device return its status.
This is one of two commands which will force a response from the client.

The command, as sent by the master to the client is comprised of three
bytes:

1. The response channel - the radio channel the client
should use to transmit its status.
2. The node ID of the receiving station on that channel.
3. The status type - the application-specific status information requested
by the controller.

Multiple different types of custom status messages are supported.
The first two (RADIO_STATUS_DYNAMIC and RADIO_STATUS_STATIC) need
to be implemented by the client.
Any additional status types are considered application-specific.
Static status includes things like the four-byte device ID, and
the firmware revision.
These values don't change.
The dynamic configuration includes battery voltage, operational
state, and other parameters which change over time.
Additional status types can be implemented and they will be
forwarded (but ignored) by the master layer.

The main controller will request a dynamic status from
all active devices on a frequent basis and without
any upstream prompting.

Payload: 2 bytes: cc nn tt

The command requires a response channel (*cc*),
a node address (*nn*), and a status type (*tt*).
The response packet from the client will have a command
type of RADIO_STATUS_RESPONSE and will be broadcast on
the specified channel.

## Command: RADIO\_CMD\_ACTIVATE

To activate a client, a broadcast message is sent on channel 0.
This message has a payload of six bytes, which are defined as follows:

1. Client channel ID
2. Client node ID
3. Selected client category byte #1
4. Selected client category byte #2
5. Selected client instance byte #1
6. Selected client instance byte #2

The channel ID tells the client to move away from channel 0 for future
command reception, as channel 0 is usually used for low-volume traffic
announcing activation messages only.
However, this is a convention and channel 0 can actually be used
for all communications, if desired.

The client node ID will be used by the main controller to identify this
particular station as long as it remains active.
The four bytes of node selection are defined in the
Client Activation setion of the main
[README](./README.md) file.
Essentially, it is a system-unique four byte identification code for
the particular device.

Payload: 6 bytes: cc nn c1 c2 n1 n2

## Command: RADIO\_CMD\_DEACTIVATE

The deactivate command simply forces the client device to deactivate,
releasing its node ID and moving to a WARM state.
The node ID can be re-used, after some form of quarantine period.
The command takes no arguments.

Payload: 0 bytes.

## Command: RADIO\_CMD\_SET\_TIME

This command is used to synchronize the tens\_of\_minutes clock on the
main controller with one or more of the clients.
Generally it is sent as a broadcast to all clients, but this is not
essential.
The actual millisecond/second time is always sent in every packet and
received by the client for internal use.

The time and date functionality of the system is a little unorthodox.
Read the section on **Time and Date** 
in the [README](./README.md) file for more information.

Payload: 1 byte: td

The command has a data length of one byte which is the time of
day (*td*) in the range of 0 -> 143.
It is also possible to disable clock functionality across the entire
system by broadcasting a SET\_TIME command with a value of 0xff.
However, this doesn't appear to be entirely useful.

## Command: RADIO\_CMD\_SET\_DATE

The date is not used or incremented by the library or by the main
controller.
However, the value can be transmitted from time to time
during the day in case any client application needs the
information.

Payload: 2 bytes: dh dl

The date is stored as an application-specific 16-bit value, if
required.
An example might be the number of days since January 1st, 2000.

## Command: RADIO\_CMD\_READ\_EEPROM

This command will read up to sixteen (16) bytes of EEPROM data and send
it in a response packet.
It takes four arguments:

1. The response channel - the radio channel the client
should use to transmit its status.
2. The number of bytes to read.
3. The upper 8 bits of the EEPROM address for reading.
4. The lower 8 bits of the EEPROM address for reading.

Similar to the CMD\_STATUS above, the data is returned on an out-of-band
channel.
The client will respond with the EEPROM data using a
command type of RADIO_EEPROM_RESPONSE.

Payload: 5 bytes: cc ll ah al

The response is sent on channel *cc* to node *nn*.
It is comprised of *ll* bytes of EEPROM data,
starting at address *ah/al*.

## Command: RADIO\_CMD\_WRITE\_EEPROM

This command will write up to sixteen (16) bytes of EEPROM data.
It takes 2 arguments and at least one byte of EEPROM data.

2. The upper 8 bits of the EEPROM address for writing.
3. The lower 8 bits of the EEPROM address for writing.

The remaining bytes in the packet are for the 1 to 16 bytes of data.

As with all the radio commands, there is no confirmation that the
operation has been received, much less completed.
Use the RADIO\_CMD\_READ\_EEPROM to verify EEPROM contents are
as anticipated.

Payload: 3-22 bytes: ah al nn [nn nn ...]

## Command Response: RADIO\_STATUS\_RESPONSE

The response packet contains status-specific data, but the two standard
status responses are as detailed in the next two subsections.

### RADIO\_STATUS\_DYNAMIC

Payload: 3-11 bytes: ss bb bb b0 b1 b2 b3 b4 b5 b6 b7

The battery voltage is represented as a 10, 12 or 16 bit value (*bbbb*).
As different boards can have different battery voltages, different
voltage divider circuits and different resistance tolerances, the voltage
is returned as a raw analog-to-digital value, for interpretation by
upper-level code.
The current operational state is returned as a single byte (*ss*).
The last eight bytes (*b0-7*) are optional and application-specific.

### RADIO\_STATUS\_STATIC

Payload: 6 bytes: c1 c2 n1 n2 fh fl

The category bytes (*c1* and *c2*) along with the instance bytes
(*n1* and *n2*) form the world-unique identification of the device.
The current firmware version is stored in two bytes (*fh* and *fl*)
which represent the version in the form *fh*.*fl* or v2.8 for example.

This status response can be used to detect if the remote device needs
a firmware upgrade.

## Command Response: RADIO\_EEPROM\_RESPONSE
