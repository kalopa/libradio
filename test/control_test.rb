#!/usr/bin/env ruby
#
require 'serialport'

if ARGV.size != 2
  STDERR.puts "Usage: control_test.rb <device> <speed>"
  exit(2)
end

p ARGV[0]
p ARGV[1].to_i
sp = SerialPort.open(ARGV[0], ARGV[1].to_i)
sp.read_timeout = 1000
p sp

exit(0)

open("/dev/tty", "r+") { |tty|
  tty.sync = true
  Thread.new {
    while true do
      tty.printf("%c", sp.getc)
    end
  }
  while (l = tty.gets) do
    sp.write(l.sub("\n", "\r"))
  end
}

sp.close
