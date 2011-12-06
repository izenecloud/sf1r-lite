#!/usr/bin/ruby -w
# to generate initial purhcase data for recommendation,
# convert from index SCD files to user and order SCD
#
# Author:: Jun
# Date:: 2011-12-05

require 'scd_scanner'
require 'category_counter'
require 'user_order_generator'

if ARGV.size < 1
  puts "usage: #{$0} index_scd_dir"
  puts "(you need to specify \"index_scd_dir\" as the directory containing index SCD files"
  exit 1
end

DOCID = "DOCID"
CATEGORY = "Category"
MAX_LEVEL = 0 
CAT_DELIM = ">"

indexDir = ARGV[0]
scanner = SCDScanner.new(indexDir, DOCID, CATEGORY)
counter = CategoryCounter.new(MAX_LEVEL, CAT_DELIM)

puts "counting docs for each category..."
scanner.scan do |docId, catList|
  counter.add(catList)
end

timeStamp = Time.now.strftime("%Y%m%d%H%M-%S")
userFile = "B-00-#{timeStamp}000-I-C.SCD"
orderFile = "B-00-#{timeStamp}111-I-C.SCD"

generator = UserOrderGenerator.new(userFile, orderFile, counter)
puts "\ngenerating users and orders..."
scanner.scan do |docId, catList|
  generator.add(docId, catList)
end
generator.close

puts "\ngeneration is finished:"
puts "#{generator.userCount} users in #{userFile}"
puts "#{generator.orderCount} orders in #{orderFile}"
