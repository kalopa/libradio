# Simple receive monitor

This code make a simple snooping device to listen for any and all traffic
on a given channel.
Useful for debugging bidirectional communications.
For simple operation, the listen-channel is either hard-coded or in
EEPROM.

## Commands

Any key input is handed to libradio\_debug() for processing.
See the file `DEBUG_CMDS.md` in the libradio directory for more
information.
