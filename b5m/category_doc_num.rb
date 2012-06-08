#!/usr/bin/env ruby

require 'yaml'
require 'fileutils'
require 'sf1-util/scd_parser'

Category = Struct.new(:category, :count)
scd = ARGV[0]
category_map = {}
parser = ScdParser.new(scd)
n = 0
parser.each do |doc|
  n+=1
  if n%1000==0
    puts "processing #{n}"
  end
  category = doc['Category']
  next if category.nil?
  category.strip!
  next if category.empty?
  if !category_map.has_key?(category)
    category_map[category] = 1
  else
    category_map[category] += 1
  end
end
category_list = []
category_map.each_pair do |c, n|
  category_list << Category.new(c, n)
end
category_list.sort! { |a, b| a.count <=> b.count }
category_list.each do |c|
  STDOUT.puts "#{c.category}\t#{c.count}"
end

