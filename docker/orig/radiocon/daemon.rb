#!/usr/bin/env ruby
#
# Copyright (c) 2019-25, Kalopa Robotics Limited.  All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# ABSTRACT
# This is the upper-level interface to the radio controller. Whereas
# the controller itself manages the radio transmissions and receiving
# status, the high-level code arranges for queued messages to be sent,
# and responses to be published. It uses RabbitMQ as the message-passing
# layer. See the README.md file for more information.
#
require 'bunny'
require 'serialport'
require 'json'
require 'prometheus/client'
require 'prometheus/client/push'

#
# For every active node, retain some useful information
class ActiveNode
  attr_accessor :channel, :node_id, :device_id
  attr_reader :last_status_request, :last_status_response

  #
  # Initialize the instance
  def initialize(c1, c2, n1, n2)
    @device_id = [c1, c2, n1, n2]
    @channel = 0
    @node_id = 0
    @last_status_request = nil
    @last_status_response = nil
  end

  #
  # Node activation data
  def activation_data
    [@channel, @node_id, @device_id.flatten]
  end
end

#
# The main work is done by this class.
class RadioCon
  COMMANDS = [
    "No-OP",
    "Firmware Request",
    "Status Request",
    "Activate!",
    "Deactivate",
    "Set Time",
    "Set Date",
    "Read EEPROM",
    "Write EEPROM",
    "Status Response",
    "EEPROM Response"
  ].freeze

  #
  # Initialize the instance
  def initialize
    @rx_run = @tx_run = false
    @radio_state = @tens_of_minutes = @battery_voltage = 0
    @npacket_rx = @npacket_tx = 0
    @controller_id = [0, 0, 0, 0]
    @firmware_version = "v0.0"
    @connection = Bunny.new(host: ENV['RABBITMQ_NODE'],
                           vhost: ENV['RABBITMQ_VHOST'],
                           user: ENV['RABBITMQ_USER'],
                           pass: ENV['RABBITMQ_PASS'])
    @connection.start
    @device = SerialPort.open(ENV['SERIAL_DEVICE'], ENV['SERIAL_SPEED'].to_i)
    @device.read_timeout = 10000
    @cmd_good = 0
    @active_nodes = []
    @next_node_id = 0
    @my_node_id = 0
  end

  #
  # Run the receive and transmit threads separately.
  def run_threads
    @rx_run = @tx_run = true
    puts "Starting receive thread..."
    rxth = Thread.new {rx_thread}
    puts "Starting transmit thread..."
    txth = Thread.new {tx_thread}
    puts "Waiting for threads to finish..."
    txth.join
    puts "TX thread has exited!"
    rxth.join
    puts "RX thread has exited!"
  end

  #
  # Receive status from the control device and broadcast the information.
  def rx_thread
    puts "RX: Waiting for control device messages..."
    while @rx_run and @tx_run do
      @device.each_line do |line|
        line = line.scrub('.').chomp
        puts "RX:#{Time.now.strftime('%T')}: #{line}"
        next unless line =~ /^</
        response = line[1..-1].split /:/
        if response[0] == "A#{@my_node_id}" and response[2].to_i == 9
          local_status(response[3])
        elsif response[0] =~ /^-/
          puts "*** Bad! ***"
          @cmd_good = 1
        elsif response[0] == "+"
          puts "Good."
          @cmd_good = 2
        else
          puts "Response: #{response}"
        end
      end
      @rx_run = false
      @cmd_good = 1
    end
    puts "RX: Thread has terminated."
  end

  #
  # Wait for RabbitMQ messages and send them to the control device.
  def tx_thread
    @channel = @connection.create_channel
    @queue = @channel.queue('channel0_tx')

    puts "TX: Performing control device setup..."
    setup
    puts "TX: Waiting for RabbitMQ messages..."
    node = ActiveNode.new(0x7f, 1, 0, 1)
    #node.
    while @rx_run and @tx_run do
      client_activate([1, 3, 0x7f, 0x01, 0, 1])
      request_status(1, 3, 0)
      request_status(1, 3, 1)
      puts "Wait..."
      sleep 5
    end
    #begin
    #  @queue.subscribe(block: true) do |_delivery_info, _properties, body|
    #    puts "Received '#{body}'."
    #    p _delivery_info
    #    p _properties
    #    p body
    #  end
    #rescue Interrupt => _
    #  puts "FAIL!"
    #  @connection.close
    #  exit 0
    #end
    @tx_run = false
    puts "TX: Thread has terminated."
  end

  #
  # Do the control device setup. This involves getting it out of a reset
  # state if it's in one, and configuring the basic essentials.
  def setup
    reset
    sleep 2
    main_activate
    @my_node_id = 1
    set_time
    set_date
    set_channel(0, 2)
    set_channel(1, 2)
    set_channel(2, 2)
    set_channel(3, 1)
  end

  #
  # Local status message
  def local_status(status)
    args = status.split /,/
    args.map! {|v| v.to_i}
    case args[0]
    when 0
      @radio_state = args[1]
      @tens_of_minutes = args[2]
      @battery_voltage = args[3] << 8 + args[4]
      @npacket_rx = args[5] << 8 + args[6]
      @npacket_tx = args[7] << 8 + args[8]
      puts ">>Radio state: #{@radio_state}."
      puts ">>Tens of Minutes: #{@tens_of_minutes}."
      puts ">>Battery voltage: #{@battery_voltage}."
      puts ">>RX/TX: #{@npacket_rx}, #{@npacket_tx}."
    when 1
      @controller_id = args[1..4]
      @firmware_version = "v#{args[5]}.#{args[6]}"
      puts ">>Controller ID: #{@controller_id}."
      puts ">>Firmware: #{@firmware_version}"
    else
      puts ">>?Unknown local status."
    end
  end

  #
  #
  def main_activate
    puts "Main activation."
    send_command chan: 0, node: 0, cmd: 3, data: [0, 1, 0].flatten
    wait_response
    get_local_status
    wait_response
  end

  #
  # Activate the client device
  def client_activate(args)
    send_command chan: 0, node: 0, cmd: 3, data: args.flatten
    wait_response
  end

  #
  # Request device status
  def request_status(chan, node, type)
    send_command chan: chan, node: node, cmd: 2, data: [3, 1, type].flatten
    wait_response
  end

  #
  #
  def set_time
    tstamp = Time.now
    ticks = ((tstamp.to_f * 100.0) % 60000.0).to_i
    tod = ((tstamp.to_f / 600.0) % 144.0).to_i
    puts "Time: #{tod}-#{ticks}"
    send_command node: 1, cmd: 5, data: [ticks % 256, ticks / 256, tod]
    wait_response
    get_local_status
    wait_response
  end

  #
  #
  def set_date
    tstamp = Time.now
    date = ((tstamp.year - 2000) * 12 + (tstamp.mon - 1)) * 32 + tstamp.day
    puts "Date: #{date}"
    send_command node: 1, cmd: 6, data: [date % 256, date / 256]
    wait_response
    get_local_status
    wait_response
  end

  #
  #
  def set_channel(channo, state)
    send_command chan: 0, node: 1, cmd: 16, data: [channo, state].flatten
    wait_response
    get_local_status
    wait_response
  end

  #
  # Get the local device status.
  def get_local_status(type = 0)
    if type == 0
      @device.puts ">S"
    else
      @device.puts ">T"
    end
  end

  #
  # Reset the controller.
  def reset
    puts "RESET CONTROLLER"
    @device.puts ">R"
  end

  #
  # Send a command
  def send_command(opts = {})
    @cmd_good = 0
    opts[:chan] ||= 0
    opts[:node] ||= 0
    str = ">#{(opts[:chan] + 65).chr}#{opts[:node]}:#{opts[:cmd]}"
    str += ":#{opts[:data].join(',')}" unless opts[:data].nil?
    str += ".\r\n"
    puts "Command: #{COMMANDS[opts[:cmd]]}, node: #{opts[:node]}, chan: #{opts[:chan]}."
    puts "SEND: [#{str.chomp}]"
    @device.puts str
  end

  #
  # Wait for some sort of response
  def wait_response
    puts "TX: Wait for response..."
    while @cmd_good == 0 do
      sleep 1
    end
    puts "Resp: #{@cmd_good}."
    return @cmd_good == 2
  end

  #
  # Assign the next available node_id to the active node.
  def assign_node
  end
end

#
# Check the environment variables are all set up properly.
envs = %w{RABBITMQ_NODE RABBITMQ_VHOST RABBITMQ_USER RABBITMQ_PASS SERIAL_DEVICE SERIAL_SPEED}

bad_env = false
envs.each do |env_var|
  next unless ENV[env_var].nil?
  unless bad_env
    puts "The following environment variables are not defined:"
    bad_env = true
  end
  puts "  $#{env_var}"
end
exit 2 if bad_env

#
# ENV is good. Initialize the RadioCon class and fire up the handler
# threads.
rc = RadioCon.new
rc.run_threads
exit 0
