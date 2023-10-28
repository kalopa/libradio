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
    puts "#{Time.now.strftime('%T')}: #{line}"
  end
  puts "RX Done."
end

#
# Writer thread.
def transmit_thread(sp)
  device_activated = false
  reset(sp)
  sleep 2
  activate(sp)
  lcount = 0
  loop do
    puts "Loop ##{lcount}..."
    case lcount
    when 1
      puts "Set time..."
      set_time(sp)
    when 2
      puts "Set the date..."
      set_date(sp)
    when 4
      puts "Activate channel 0..."
      set_channel(sp, 0, 2)
    when 5
      puts "Activate channel 1..."
      set_channel(sp, 1, 2)
    when 6
      puts "Activate channel 2..."
      set_channel(sp, 2, 2)
    when 7
      puts "Activate channel 3..."
      set_channel(sp, 3, 1)
    end
    if lcount > 9
      case lcount & 07
      when 0
        sp.puts ">S\r\n"
      when 1
        sp.puts ">T\r\n"
      when 2
        client_activate(sp, [1, 3, 0x7f, 0x01, 0, 1])
      when 3
        request_status(sp, 1, 3, 0)
      when 4
        request_status(sp, 1, 3, 1)
      end
    end
    lcount += 1
    sleep 5
  end
end

#
#
def activate(sp)
  send_command sp, chan: 0, node: 0, cmd: 3, data: [0, 1, 0].flatten
end

#
# Activate the client device
def client_activate(sp, args)
  send_command sp, chan: 0, node: 0, cmd: 3, data: args.flatten
end

#
# Request device status
def request_status(sp, chan, node, type)
  send_command sp, chan: chan, node: node, cmd: 2, data: [3, 1, type].flatten
end

#
#
def set_time(sp)
  tstamp = Time.now
  ticks = ((tstamp.to_f * 100.0) % 60000.0).to_i
  tod = ((tstamp.to_f / 600.0) % 144.0).to_i
  puts "Time: #{tod}-#{ticks}"
  send_command sp, node: 1, cmd: 5, data: [ticks % 256, ticks / 256, tod]
end

#
#
def set_date(sp)
  tstamp = Time.now
  date = ((tstamp.year - 2000) * 12 + (tstamp.mon - 1)) * 32 + tstamp.day
  puts "Date: #{date}"
  send_command sp, node: 1, cmd: 6, data: [date % 256, date / 256]
end

#
#
def set_channel(sp, channo, state)
  send_command sp, chan: 0, node: 1, cmd: 16, data: [channo, state].flatten
end

#
# Send a command
def send_command(sp, opts = {})
  opts[:chan] ||= 0
  opts[:node] ||= 0
  str = ">#{(opts[:chan] + 65).chr}#{opts[:node]}:#{opts[:cmd]}"
  str += ":#{opts[:data].join(',')}" unless opts[:data].nil?
  str += ".\r\n"
  puts "SEND: [#{str.chomp}]"
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
