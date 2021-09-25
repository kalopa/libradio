# Signalling Protocol

The signalling network is comprised of a single master node and multiple (up to 239)
slave nodes, such as locomotives, automated points, and so on.
The network operates on the 433MHz telemetry radio frequency, in half-duplex
mode.
It runs at a speed of 9600 baud, with no parity and two stop bits.

The master node sends a frame out on the air, and may or may not wait for
a response from the slave.
The frame communication window is 20ms.
Generally the master will send a frame every 20ms, and the slave will
respond.
Note that only 48 frames are sent, so the last two frame slots are empty.
If the master sends a 7 byte frame, at 9600 baud, this will take 7.3ms
approximately.
The slave then has a 12.7ms time window to respond,
before the master will send the next frame.
The slave should not commence transmission if it has been more than 5ms
since the last byte of the master frame was received.

It is possible, due to the escape byte sequences and certain worst-case
payloads, that the frame is comprised of eleven (11) bytes.
This can occur if all three data bytes and the checksum byte are in the
illegal range (0xc0 -> 0xc3) and need to be escaped.
It is also equally feasible, but equally unlikely that the slave response
could also be 11 bytes, in which case the 20ms window will be violated.
In this rare case, the next 20ms window will be ignored and the whole
frame will take 40ms to send.

The slave node should not respond unless the following conditions are met:

1. The frame was received with a correct checksum.
2. The received frame is uniquely addressed to the slave.

In other words, no response is expected from a broadcast.

## Frame Format

The frame format is as follows:

| Byte | Description |
| ---- | ----------- |
| 0xc0 | Header byte |
| dd | Destination |
| cmd | Command |
| d1 | First data byte (optional) |
| d2 | Second data byte (optional) |
| d3 | Third data byte (optional) |
| csum | Checksum |

The smallest command frame is four (4) bytes and the largest is seven (7) bytes.
Sent at 9600 Baud, no parity, two stop bits.

### Frame Header

The header byte (*0xc0*) can only be sent as a header byte.
In other words, if 0xc0 is seen in the data stream, then expect
the command frame destination to follow.
See the section below on escape sequences in the event that an actual 0xc0 byte needs
to be sent.

### Frame Destination

The destination is one of a broadcast, multicast or unicast address.
Each node on the network has a unique node ID in the range 0x00 to 0xef
(note that the node addresses in the 0xc0 to 0xc3 range are never used).
The master node is always address 0x00 but this address is never actually
used on the network.
Generally, a node which does not have an address (it is unassigned)
will use the 0x00 address.
An address of 0xff means "all nodes."
Addresses in the range 0xf0 through 0xfe are broadcast messages to a specific
class of node, as follows:

| Address | Type |
| ------- | ---- |
| 0xf0 | Unassigned nodes |
| 0xf1 | Track-point nodes |
| 0xf2 | Signal box nodes |
| 0xf3 | Locomotive nodes |
| 0xf4 | Rolling stock nodes|
| 0xf5 | Train station nodes |
| 0xf6-0xfe | **Reserved** |
| 0xff | Broadcast - "all nodes" |

### Command Byte

The command byte can be one, two or three bytes long.
As a short-hand, the last two bits of the command byte specify
the number of data bytes to follow.
For example, the command 0x42 will be followed by two data bytes.
This allows the low-level receiver code to know the packet length
without understanding the command format.

Some commands are reserved as "extended" commands.
For example, the command byte 0x01 (with one data byte)
dictates that the data byte is the extended command and there is no
actual data associated with it.
The command byte 0x02 (a command with two data bytes)
means that there is an extended command with one associated data byte,
and the command byte 0x03 is an extended command with two
data bytes.
The special command of 0x00 is a no-op.
Any frame received with that command, can be ignored.

Commands have different meanings depending on whether they are sent from
the master station (node ID 0x00) to a slave station, or
are a response from a slave station to the master.
For example, the command 0x04 (no data bytes) is a PING.
The correct response from the slave is an ACK (which is also 0x04).
Note that the response frame sets the destination field (**dd**) to be
its node address, not the node address of the master.

See the [Signalling Commands](./COMMANDS.md) for more information on
the actual commands.

## Escape Bytes

The four bytes in the range 0xc0 through 0xc3 are reserved and can never be sent
in a frame.
These node addresses are reserved, so cannot appear as a destination or source
address.
The command bytes are also reserved, so cannot appear as a command.
To send a reserved byte in the data fields (or as a checksum),
first send 0xc3 and then the seven least-significant bits of the required
byte.
As an example, to send the data byte 0xc0, you would send the sequence
0xc3, 0x40.
The receiver code, once it has done the escape byte processing, will
restore the most significant bit of the received byte.

The special escape-byte 0xc1 is used to alternate into "data" mode.
This is useful for debug messages and the like.
Once a 0xc1 byte is received, the receiver will log all bytes until
a header byte is received.
Generally the data bytes are seven bit, but if there is a requirement
to send a reserved byte, it is escaped as above.

## Node ID Assignment

Each node will have its node type saved in firmware.
These types are as follows:

| Type | Number Type | Name |
| ---- | ----------- | ---- |
| 0 | N/A | Master node |
| 1 | ID | Track-side nodes |
| 2 | ID | Train station nodes |
| 3 | Road Number | Locomotive nodes |
| 4 | Road Number | Rolling stock nodes|
| 5 | Road Number | Special-purpose Vehicle |

In addition, in EEPROM, it will have a sixteen bit road number or ID
assigned, depending on the type (see the table above).
In summary, the world-unique identification of the node is
the combination of the node type and the 16 bit number.

On cold boot, a node will set its ID to zero (0x00) and will know that it
is unassigned.
It will only listen to broadcast frames (**dd** == *0xff*) and frames
addressed to unassigned nodes (**dd** == *0xf0*).
The broadcast frames will allow the node to capture the date and time,
as well as the current millisecond clock in use on the network.

For the system to activate a node, and bring it online, a special
activation message is sent (see the Signalling Commands for the specifics).
This activation message is unique to the node type (as above), includes
the sixteen bit ID of the node, and the newly-assigned Node ID.
As the device may be "sleeping," it would not be unusual for the master
node to have to send the activation message multiple times over the course
of several minutes.
Generally, as a rule, activation messages are broadcast (to all unassigned
nodes) directly after the millisecond clock transmission, or **Frame 1** in
other words.
Devices which are in a standby mode, can sleep for several minutes at a time,
waking up only to maintain their real-time clocks.
Every few minutes, they will enable their radios and receive a series of
packets, including one or more after the millisecond clock broadcast.
If they receive an activation message, they will save the node ID and move
to an active state, listening for all further communications.

The master node will know that a slave node has received its activation
when it responds to a ping command.
So the master node will follow up every activation with a ping command
a short while after, and if no response is received, the activation will
be retransmitted.
Eventually, the master node will give up attempting to activate the node.

Similarly, there is a simple (non-broadcast) command to tell a node to
go from an active to standby mode.
A positive acknowledgement (ACK) from the slave indicates that this will
happen, and no further communication on that node ID is permitted, until
a subsequent activation command.
To simplify this process and to prevent race conditions whereby status
requests and pings to the slave node interfere with a deactivation
command, the node is "silenced" for a period of time prior to the
deactivation.
Ideally, no frames of any type with that node ID would be sent for a
period of minutes before the deactivation.
