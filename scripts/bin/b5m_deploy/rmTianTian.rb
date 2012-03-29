#!/usr/bin/env ruby
require 'rubygems'
require 'require_all'
require 'sf1-util/scd_parser'
require 'sf1-util/scd_writer'
scd_file = ARGV[0]

parser = ScdParser.new(scd_file)
writer = ScdWriter.new("./", 'D')
count = 0
parser.each do |doc|
  count += 1
  puts "doc count #{count}" if count%1000==0
  source = doc['Source']

  #puts "#{price}  :  #{min_price}"
  if source=="鐢崇綉瀹剁數鍩�"
    new_doc = {}
    new_doc['DOCID'] = doc['DOCID']
    writer.append(new_doc)
    next
  end
end
writer.close
