#!/usr/bin/env ruby

require 'rubygems'
require 'require_all'
require 'sf1-util/scd_parser'
require 'sf1-util/scd_writer'

scd_file = ARGV[0]

parser = ScdParser.new(scd_file)
count = 0
parser.each do |doc|
  count += 1
  puts "doc count #{count}" if count%1000==0
  discount = doc['Discount']
  priceOrigin = doc['PriceOrigin']	
  price = doc['Price']
  f_priceOrigin = priceOrigin.to_f
  f_price = price.to_f
  #puts "#{price}  :  #{min_price}"
#  discount = #{f_price} / #f_priceOrigin * 10 
  puts "price : #{price}"
  puts "discount : #{discount}"
end
writer.close
