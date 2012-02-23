#!/usr/bin/env ruby

require 'rubygems'
require 'yaml'
require 'fileutils'

category_file = ARGV[0]
scd = ARGV[1]
IO.readlines(category_file).each do |c_line|
  c_line.strip!
  next if c_line.empty?
  a = c_line.split(',')
  category_regex = a[0]
  cid = a[1]
  category_regex = category_regex[1..-1] if category_regex.start_with?("#")
  category_regex = category_regex[1..-1] if category_regex.start_with?("^")
  df = `grep -c '<Category>#{category_regex}' #{scd}`
  STDOUT.puts "#{cid} - #{category_regex} : #{df}"
end

