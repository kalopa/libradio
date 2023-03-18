#!/usr/bin/env ruby
#
require 'serialport'

DEVICE = "/dev/ttyS0"
SPEED = 38400

#
# Initialize the radio controller.
def setup(sp)
end

#
# Receiver thread.
def receive_thread(sp)
  sp.each_line do |line|
    line = line.scrub('.').chomp
    p line
  end
  puts "RX Done."
end

#
# Writer thread.
def transmit_thread(sp)
  reset(sp)
  sleep 2
  activate(sp)
  set_time(sp)
  set_date(sp)
  5.times do |i|
    set_channel(sp, i)
  end
  while true do
    sp.puts ">S\r\n"
    sp.puts ">T\r\n"
    sleep 5
  end
end

#
#
def activate(sp)
  send_command sp, chan: 0, node: 0, cmd: 3, data: [0, 0, 0].flatten
end

#
#
def set_time(sp)
  tstamp = Time.now
  ticks = ((tstamp.to_f * 100.0) % 60000.0).to_i
  tod = ((tstamp.to_f / 600.0) % 144.0).to_i
  puts "Time: #{tod}-#{ticks}"
  send_command sp, cmd: 5, data: [ticks % 256, ticks / 256, tod]
end

#
#
def set_date(sp)
  tstamp = Time.now
  date = ((tstamp.year - 2000) * 12 + (tstamp.mon - 1)) * 32 + tstamp.day
  puts "Date: #{date}"
  send_command sp, cmd: 6, data: [date % 256, date / 256]
end

#
#
def set_channel(sp, channo)
  send_command sp, chan: 0, node: 0, cmd: 16, data: [channo, 2].flatten
end

#
# Send a command
def send_command(sp, opts = {})
  opts[:chan] ||= 0
  opts[:node] ||= 0
  str = ">#{(opts[:chan] + 65).chr}#{opts[:node]}:#{opts[:cmd]}"
  str += ":#{opts[:data].join(',')}" unless opts[:data].nil?
  str += ".\r\n"
  puts "SEND: [#{str}]"
  sp.puts str
end

#
# Reset the controller.
def reset(sp)
  puts "RESET CONTROLLER"
  sp.puts ">R"
end

sp = SerialPort.open(DEVICE, SPEED)
sp.read_timeout = 10000
p sp

setup(sp)
rxth = Thread.new {receive_thread(sp)}
txth = Thread.new {transmit_thread(sp)}

txth.join
rxth.join

exit(0)
