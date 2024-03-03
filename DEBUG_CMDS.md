
# Debug Commands

Any key input is handed to libradio\_debug() for processing.
The commands are explained in the following sections.
The command output usually starts with `N>` where *N* is the current
receive channel.

## Key: 0-9

Switch radio reception to the numbered channel (0 through 9).
Higher-order channels aren't currently supported.

Output:

   -> X

New receive channel is X.

## Key: r

Reset and switch to bootstrap mode.
Useful for reprogramming the firmware over the serial link.

## Key: i

Enable radio interrupts by calling `libradio_irq_enable(1);`.
Output:

    IRQ-ena

## Key: o

Disable radio interrupts by calling `libradio_irq_enable(0);`.
Output:

    IRQ-dis

## Key: h

Call `libradio_handle_packet()` to receive a packet and deal with it.
Output:

    handle packet

## Key: p

Get the Si4463 part information by calling `libradio_get_part_info();`.
Output:

    PART: <f1>/<f2>/<f3>/<f4>/<f5>/<f6>

Where *f1* is `chip_rev`, *f2* is `part_id`, *f3* is `pbuild`,
*f4* is `device_id`, *f5* is `customer` and *f6* is `rom_id`

## Key: f

Get the Si4463 functional information by calling
`libradio_get_func_info();`.
Output:

    FUNC: <f1>/<f2>/<f3>/<f4>/<f5>

Where *f1* is `rev_ext`, *f2* is `rev_branch`, *f3* is `rev_int`,
*f4* is `patch` and *f5* is `func`.

## Key: c

Get the Si4463 chip status by calling `libradio_get_chip_status();`.
Output:

    CHIP:<f1>/<f2>/<f3>

Where *f1* is `chip_pending`, *f2* is `chip_status` and
*f3* is `cmd_error`.

## Key: d

Get the Si4463 device status by calling `libradio_request_device_status();`.
Output:

    DEVST:<f1>,CH<f2>

Where *f1* is `curr_state` and *f2* is `curr_channel`.

## Key: s

Get the Si4463 interrupt status by calling `libradio_get_int_status();`.
Output:

    INTST: I<f1>/<f2>,P<f3>/<f4>,M<f5>/<f6>,C<f7>/<f8>

Where *f1* is `int_pending`, *f2* is `int_status`, *f3* is
`ph_pending`, *f4* is `ph_status`, *f5* is `modem_pending`, *f6*
is `modem_status`, *f7* is `chip_pending` and *f8* is `chip_status`.

## Key: g

Get the Si4463 FIFO information by calling `libradio_get_fifo_info(0);`.
Output:

    FIFO RX:<f1>,TX:<f2>

Where *f1* is `rx_fifo` and *f2* is `tx_fifo`.

## Key: z

Calibrate the radio by calling `libradio_ircal();`.

## Key: P

Get the property defined by `libradio_get_property(0x100, 4);`.
