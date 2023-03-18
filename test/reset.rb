#!/usr/bin/env ruby
#
require 'serialport'

DEVICE = "/dev/ttyS0"
SPEED = 38400


sp = SerialPort.open(DEVICE, SPEED)
sp.read_timeout = 10000
p sp
puts "RESET CONTROLLER"
sp.puts ">R"
exit(0)
