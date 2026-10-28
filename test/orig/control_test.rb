#!/usr/bin/env ruby
#
require 'serialport'

$timeout = nil

COMMAND_NOOP = 0
COMMAND_FIRMWARE = 1
COMMAND_STATUS = 2
COMMAND_ACTIVATE = 3
COMMAND_DEACTIVATE = 4
COMMAND_SET_TIME = 5
COMMAND_SET_DATE = 6
COMMAND_READ_EEPROM = 7
COMMAND_WRITE_EEPROM = 8
COMMAND_STATUS_RESPONSE = 9
COMMAND_EEPROM_RESPONSE = 10
COMMAND_USER0 = 16
COMMAND_USER1 = 17
COMMAND_USER2 = 18
COMMAND_USER3 = 19
COMMAND_USER4 = 20

CONTROL_ERRORS = [
  "* No Error", "Invalid Channel", "Busy", "Too Much Data", "Too Big",
  "Newline", "Radio Not Active", "Power Failure", "Bad Command"
].freeze

RADIO_STATES = [
  "Startup", "Error State", "Low Battery", "Cold Start", "Warm Start",
  "Listening", "Active"
].freeze

#
# Initialize the radio controller.
def setup(sp)
end

#
# Receiver thread.
def receive_thread(sp)
  bad_count = 0
  sp.each_line do |line|
    line = line.scrub('.').chomp
    if line =~ /^@-/
      bad_count += 1
      exit if bad_count > 10
    else
      bad_count = 0
    end
    puts "#{Time.now.strftime('%T')}: #{line} (#{bad_count})"
    $timeout = Time.now + 60
    if line =~ /^<-/
      code, state = line[2..].split /\//
      code = code.to_i
      state = state.to_i
      puts "?ERROR - #{CONTROL_ERRORS[code]} (code #{code})."
      puts "Radio state: #{RADIO_STATES[state]} (state #{state})."
      exit 1 if code != 5 #and code != 2
    end
    if line =~ /^<A1:/
      # <A1:27598:9:0,6,102,3,112,0,0,0,6 (0)
      # chno:<A1, ts: 10206, cmd: 9, args: ["0", "6", "103", "3", "112", "0", "0", "0", "20"]
      # <A1:27798:9:1,1,1,1,1,2,4 (0)
      chno, ts, cmd, data = line.split /:/
      chno = chno[1..-1]
      ts = ts.to_i
      cmd = cmd.to_i
      data = data.gsub(/ .*/, '')
      args = data.split /,/
      args.map! {|a| a.to_i}
      if cmd == 9
        #
        # Status response.
        if args[0] == 0
          bv = (args[3] << 8) + args[4] if args[3] and args[4]
          rx = (args[5] << 8) + args[6] if args[5] and args[6]
          tx = (args[7] << 8) + args[8] if args[7] and args[8]
            puts ">> Dynamic status: Radio State: #{args[1]}. ToM: #{args[2]}, Batt#{bv}. RX #{rx}, TX #{tx}."
        else
          puts ">> Static status: ID: #{args[1]}/#{args[2]}/#{args[3]}/#{args[4]}. V#{args[5]}.#{args[6]}."
        end
      end
    end
    $stdout.flush
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
    exit if lcount > 20 and $timeout and $timeout < Time.now
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
      #set_channel(sp, 1, 2)
    when 6
      puts "Activate channel 2..."
      #set_channel(sp, 2, 2)
    when 7
      puts "Activate channel 3..."
      #set_channel(sp, 3, 1)
    end
    if lcount > 9
      case lcount & 07
      when 0
        # Request dynamic status of master controller
        sp.puts ">S\r\n"
      when 1
        # Request static status of master controller
        sp.puts ">T\r\n"
      when 2
        # Activate device 34-5-35-1.
        client_activate(sp, [1, 3, 34, 5, 35, 1])
      when 3
        # Request static status
        #request_status(sp, 1, 3, 0)
        request_status(sp, 1, 3, 1)
      when 4
        # Request dynamic status
        #request_status(sp, 1, 3, 1)
        request_status(sp, 1, 3, 1)
      when 5
        # Set direction (FORWARD)
        #send_command sp, chan: 1, node: 3, cmd: COMMAND_USER1, data: [1].flatten
        request_status(sp, 1, 3, 1)
      when 6
        # Set loco speed
        #send_command sp, chan: 1, node: 3, cmd: COMMAND_USER2, data: [32].flatten
        request_status(sp, 1, 3, 2)
      end
    end
    lcount += 1
    $stdout.flush
    sleep 1
  end
end

#
#
def activate(sp)
  my_channel = 0
  my_node_id = 1
  send_command sp, chan: 0, node: 0, cmd: COMMAND_ACTIVATE, data: [my_channel, my_node_id, 0].flatten
end

#
# Activate the client device
def client_activate(sp, args)
  send_command sp, chan: 0, node: 0, cmd: COMMAND_ACTIVATE, data: args.flatten
end

#
# Request device status
def request_status(sp, chan, node, status_type)
  response_chan = 1
  response_node = 1
  send_command sp, chan: chan, node: node, cmd: COMMAND_STATUS, data: [response_chan, response_node, status_type].flatten
end

#
#
def set_time(sp)
  tstamp = Time.now
  ticks = ((tstamp.to_f * 100.0) % 60000.0).to_i
  tod = ((tstamp.to_f / 600.0) % 144.0).to_i
  puts "Time: #{tod}-#{ticks}"
  send_command sp, node: 1, cmd: COMMAND_SET_TIME, data: [ticks % 256, ticks / 256, tod]
end

#
#
def set_date(sp)
  tstamp = Time.now
  date = ((tstamp.year - 2000) * 12 + (tstamp.mon - 1)) * 32 + tstamp.day
  puts "Date: #{date}"
  send_command sp, node: 1, cmd: COMMAND_SET_DATE, data: [date % 256, date / 256]
end

#
#
def set_channel(sp, channo, state)
  send_command sp, chan: 0, node: 1, cmd: COMMAND_USER0, data: [channo, state].flatten
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

if ENV['SERIAL_DEVICE'].nil?
  puts "needs `export SERIAL_DEVICE=/dev/ttyUSB0:38400` first"
  exit 2
end
device, speed = ENV['SERIAL_DEVICE'].split /:/
sp = SerialPort.open(device, speed.to_i)
sp.read_timeout = 10000
puts "Device: #{device}"
puts "Speed: #{speed}"

setup(sp)
rxth = Thread.new {receive_thread(sp)}
txth = Thread.new {transmit_thread(sp)}

txth.join
rxth.join

exit(0)
