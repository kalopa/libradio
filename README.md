# libradio - an AVR library for 434MHz communications

This repo has the source code (and Makefile) to build a communications
network based around the Si4463 radio.
It works by using time-division multiplexing and channel allotments
so that everyone gets a chance to communicate.

The code is designed to work with the Kalopa Robotics microcontroller
board which features an Atmel ATMega328P CPU and a Si4463 radio
module.
However, any Arduino and Si4463 combination will work.
The Si4463 is quite a complicated radio and requires a lot of setup
and hand-holding.
But it is an amazing transceiver with a wide range of functions and
abilities.

To use this code with an Arduino, you need to wire the Si4463 SPI
interface to the board, along with the IRQ line.

## The Protocol Basics

The system requires a single node known as the Team Lead to synchronize
communications between the devices.
Packets are a fixed length of 16 bytes and are transmitted during a
20ms window on a specific frequency within the 434MHz band.
The first byte of the packet is the intended recipient (or 0xff for
"all stations").
The second byte is the command itself.
The remaining 14 bytes are command-specific.

Up to 255 devices can share the network at any given time.
Each is assigned an 8 bit channel ID.
Channel 0 is, by definition, the Team Lead and channel 255 is the
broadcast channel.

The time spectrum is broken up into fifty (50) packet transmissions
of 20ms each.
In other words, the pattern repeats itself every second.
Each 20ms window is referred to as a "slot".
Ten of these slots are reserved for the Team Lead's use.
At time T0 (slot #0), the Team Lead sends a BEACON command.
The command ID is 0x00 and the destination is 0xff (all stations).
The next 14 bytes indicate which channels can use which slots.
At any given time, only 14 devices can have assigned slots.

## Slot Assignments

Slot assignments are not one to one.
Some channels will be allocated five slots.
Some will be allocated two slots, and some will only have a single
slot per 50 slot window.
The last two slots (48 & 49) are reserved for offline devices.
More about them, later.
For simplicity, slots are numbered 0 through 49 and assignments are
labelled 'A' through 'N'.
Assignments A, B, C, D and E each have 5 slots assigned to them.
Assignments F, G, H and I have 2 slots.
Assignments J, K, L, M and N have a single slot.

There is a table below which identifies which assignments map to
which slots.

The BEACON command, which is always in slot 0, identifies which
8-bit channel has which assignment.
As the command is always transmitted every second, it is possible
to rearrange the slot assignments per second, allowing for
broader control of who can and cannot transmit.

## Client States

Client devices have four main states:

* ASLEEP
* AWAKE
* OFFLINE
* ONLINE

There is another state (TEAMLEAD) which is just for the Team Lead,
obviously.

Devices which are ASLEEP are not interested in taking part in
communications or listening for commands.
This may be because they only operate at certain times of the day,
or they may be solar-powered and have insufficient power for
operations.
These devices will shut down the Si4463 radio and reduce power in
as many ways as possible.
Normally, after the device starts up, it enters the AWAKE state.
In this state, it is listening for BEACON commands but has not
heard anything.
Once a BEACON is received by the radio, the synchronization timer
can be started, and the device is now in an OFFLINE state.

Devices in an OFFLINE state do not have a channel ID assigned.
All client devices have a four byte, system-unique identifier
called the Node ID.
The first two bytes dictate the class of device and the
last two bytes indicate the particular instance of device.
If the Team Lead (or more correctly, the upstream systems) is looking
for this device, it can send a HELLO packet.
The HELLO command specifies the four bytes of Node ID along with
the assigned channel.
As the 'rxchan' (recipient) of the command is invariably a broadcast
ID (0xff), this command will be received by the device even in an
OFFLINE state.
In these somewhat unusual circumstances, the offline device will
move to an ONLINE state by way of the channel assignment which
was broadcast.

Normally, however, the device will explicitly request a channel ID.
To do this, it will prepare a ONLINE_REQUEST packet and send this to
channel 0 (the Team Lead) using slot 48 or 49.
There is no mechanism for avoiding transmit collisions in either
of these two slots, so care must be taken.
These slots are not reliable.
For this reason, the ONLINE_REQUEST includes a series of checksums
and other mechanisms to detect collisions.
Also, devices would normally take one or other slot no more than
every 8 to 15 seconds, decided randomly.
This should allow new devices to transmit without a collision in
either slot, and be given a channel assignment by way of a HELLO
command.
The devices have no way of knowing if their transmission has been
successful, so they must keep trying until they hear a HELLO command
with their Node ID.
The minimum and maximum windows are configurable, so some devices
which need to get online quicker, can hog the slots (although this
is not advised).
Similarly, devices which aren't in a hurry, can push out the 8-15
second window, to allow other devices a better chance at getting
through.

Once a HELLO pakcet is received, with a matching 4 byte Node ID,
the client will save its new channel ID and move to an ONLINE
state.
Note that even in an ONLINE state, the device cannot transmit
until it is given a slot assignment letter.

## Transmission Synchronization

As the radio timing is quite tight, it is important that each device
transmits as close to the centre of their alotted slot as possible.
Transmission is started in the main loop rather than in an interrupt
service routine, due to the SPI communications.

At some point, I would like to be smarter about pre-loading the
transmit FIFO, but for now, nothing happens until the timer interrupt
sets a flag to tell the main code it can commence transmitting.

Synchronization is worked off the TIMER1 timer on the chip.
It is a free-running clock, running at 10kHz.
When the library is initialized, the TIMER1 clock is configured
and started.
Each transmit slot then occupies 200 units of the timer.
As there is latency from the time the Team Lead prepares and transmits
the BEACON packet, the delay inherent in the radio chip, as well
as the transmission and reception delays, by the time the client
receives the BEACON, some number of clock ticks have elapsed.

This delay has been determined empirically and programmed into the
code and saved as a configuration variable.
If a BEACON command is received, the the latency parameter is
programmed into TIMER1, so that it is now a very close approximation
to the TIMER1 value running on the Team Lead.
It is possible to apply an adjustment factor to this configuration
variable, if the client code is faster or slower at processing the
BEACON command, or faster/slower in scheduling a transmission.
For the most part, the default value is sufficient.

As an example, if it takes 18ms from the time the Slot 0 interrupt
happens on the client, until the BEACON command is received on the
client, then the TIMERR1 value will be set to 180.
The first slot then will open up in a mere 2ms (or 20 ticks) from
receipt of the BEACON.

Once the BEACON is received by the client, if it has been given a
slot allocation, the client will determine the next available
transmission slot for it, and configure the TIMER1 interrupt
accordingly.
Once that time is reached, a flag is set and the next allocation
is determined and the interrupt reconfigured. If no more slots are
left, the timer is left to free-run without a slot interrupt.

In the event that the timer rolls over (after approximately 6.5
seconds), then no BEACON has been received.
It is up to the client to determine what to do in that circumstance,
but normally it will continue to listen but will not have any slot
allocations.
Should the situation continue, the client will lapse into an OFFLINE
state and the channel ID will be cleared.
At that point, the normal OFFLINE -> ONLINE mechanism will apply.

## Receive Callbacks

The client code is designed to be relatively trivial, with most of
the complexity buried in the library code.
The radio itself can be difficult to program, so this is also under
the management of the library.
The client calls `libradio_init(c1, c2, n1, n2)` to initialize the
system.
The c1/c2 values represent the device class, and the n1/n2 values
represent the instance number.
As mentioned above, the initial state is AWAKE.
If the client code will respond to any commands, it will register
a command callback using `libradio_command_callback()` giving the
name of a void function which takes a packet structure as its only
argument.
The client main loop will call `libradio_poll()` to effect the
actual radio reception and transmission.
If a packet has been received, it is pulled from the radio FIFO and
stored in a receive packet.
This packet is passed to `libradio_handle_rx()` for processing.
By convention, the first 16 commands (in the range 0x00 to
0x0f) are system commands and common to all devices.
The library code will drop the packet unless either the 'rxchan'
parameter is set to 0xff ("all stations") or the device is ONLINE
and the assigned channel number matches that of the 'rxchan' value.

The built-in function will handle commands such as BEACON, TIMESTAMP
and HELLO.
For commands not explicitly handled internally, if a command callback
has been registered, the packet will be forwarded to that function
for processing.

The command processing may involve sending a response packet.
For example, DEVICE_STATUS requires a status response packet which
is created in the command handler, and then `libradio_transmit()`
will be called to enqueue the response.
The packet isn't sent immediately, it is stored in the libradio
structure until such time as the transmit flag is set by the TIMER1
interrupt.
Note that the transmit flag is temporal.
It is possible for the flag to be set and the window is lost due
to a delay in arranging transmission.
As such, the transmission code must verify that the window is still
open.
This window size is quite small and configured in the code.

The transmit code also has a callback.
This is for multiple slot transmissions which are delivered without
the need for a request.
For example, a device with slot assignment 'A' can send five packets
per second.
The callback is used when a slot is available for transmission.
When the slot opens up, the code will first check to see if there
is an enqueued packet ready for transmission.
If so, this is given priority.
If no packet is queued up, and a slot callback has been registered,
this will be called.
Once the call returns, another check is made to see if there is an
outgoing packet.
If so, that is sent.
If no packets are lined up, the slot is skipped.

It is possible for the receive code to receive two commands which
require a response, but only one outgoing packet can be queued up.
The second packet is silently ignored.

## Team Lead Transmission

The Team Lead has ten slots assigned by default.
As discussed, slot 0 is for the BEACON.
Slot 25 is used for a TIMESTAMP command which publishes time of day
information and other general status.
The remaining eight slots can be used by upstream systems to send
HELLO packets or other transmissions.
It is also possible in certain circumstances for the Team Lead to
be given a slot assignment, but this is atypical.

The Team Lead code is very similar to the client code.
First off, the `libradio_teamlead()` function is called immediately
after the init call.
This puts the state into TEAMLEAD which is similar to the ONLINE
state, and no HELLO message is required.
The TIMER1 is automatically kicked off, which is now the main
synchronization source for the entire network.
The first interrupt is configured for 10,000 ticks (or one second).
A command callback is registered for reception of packets.
A `libradio_slot_callback()` is registered to queue any outbound
transmissions during free slots.
The argument to the callback is the slot number.
It will be one second before this is fired.
After that, the code is almost identical to the client.
The Team Lead will also listen for serial I/O and handle commands
received from the upper levels.

As a rule, and to avoid any weird side-effects, the Team Lead should
not queue a transmit packet from the command code.
The slot callback is used to transmit the BEACON command as well
as the TIMESTAMP command and any enqueued packets for transmission.

# LED Sequences

| State | LED Sequence |
| --- | --- |
| LIBRADIO\_STATE\_ASLEEP | Solid "ON" |
| LIBRADIO\_STATE\_AWAKE | On/off flashing sequence |
| LIBRADIO\_STATE\_OFFLINE | Fast flashing on/off |
| LIBRADIO\_STATE\_ONLINE | "Heartbeat" sequence |

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

## Radio Details

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

The modules I'm using for testing are marked 44631B.

## Slot Assignments

| Slot | Code |
| ---- | ---- |
| 0 | TeamLead (BEACON) |
| 1 | A |
| 2 | C |
| 3 | E |
| 4 | G |
| 5 | TeamLead (Packet #1) |
| 6 | B |
| 7 | D |
| 8 | F |
| 9 | J |
| 10 | TeamLead (Packet #2) |
| 11 | A |
| 12 | C |
| 13 | E |
| 14 | I |
| 15 | TeamLead (Packet #3) |
| 16 | B |
| 17 | D |
| 18 | H |
| 19 | K |
| 20 | TeamLead (Packet #4) |
| 21 | A |
| 22 | C |
| 23 | E |
| 24 | G |
| 25 | TeamLead (TIMESTAMP) |
| 26 | B |
| 27 | D |
| 28 | F |
| 29 | L |
| 30 | TeamLead (Packet #5) |
| 31 | A |
| 32 | C |
| 33 | E |
| 34 | I |
| 35 | TeamLead (Packet #6) |
| 36 | B |
| 37 | D |
| 38 | H |
| 39 | M |
| 40 | TeamLead (Packet #7) |
| 41 | A |
| 42 | C |
| 43 | E |
| 44 | N |
| 45 | TeamLead (Packet #8) |
| 46 | B |
| 47 | D |
| 48 | New Device |
| 49 | New Device |

The Team Lead has a total of ten (10) slots.
A through E have a total of five (5) slots each.
F through I have a total of two (2) slots each.
The remaining assignments, J through N have a single slot each.
