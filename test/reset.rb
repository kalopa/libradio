#!/usr/bin/env ruby
#
require 'serialport'

DEVICE = "/dev/ttyS0"
SPEED = 38400


if ENV['SERIAL_DEVICE'].nil?
  puts "needs `export SERIAL_DEVICE=/dev/ttyUSB0:38400` first"
  exit 2
end
device, speed = ENV['SERIAL_DEVICE'].split /:/
sp = SerialPort.open(device, speed.to_i)
sp.read_timeout = 10000
p sp
puts "RESET CONTROLLER"
sp.puts "\005\\"
exit(0)
