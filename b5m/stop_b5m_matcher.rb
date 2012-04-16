#!/usr/bin/env ruby
require 'sys/proctable'
require 'fileutils'

include Sys

top_dir = File.dirname(File.expand_path(__FILE__))
log_dir = File.join(top_dir, "log")
pid_file = File.join(log_dir, "pid")
exit unless File.exists?(pid_file)
to_kill = []
IO.readlines(pid_file).each do |line|
  line.strip!
  pid = line.to_i
  puts "find #{pid}"
  to_kill << pid
end

add_kill_count = to_kill.size
while add_kill_count>0
  add_kill_count=0
  ProcTable.ps do |p|
    next if to_kill.include?(p.pid)
    if to_kill.include?(p.ppid)
      puts "find sub #{p.pid} #{p.comm}"
      to_kill << p.pid
      add_kill_count+=1
    end
  end
end
to_kill.each do |pid|
  Process.kill(9,pid)
end
FileUtils.rm_rf(pid_file)

