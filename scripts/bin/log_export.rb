#!/usr/bin/env ruby

require 'rubygems'
require 'require_all'
require_rel '../lib/sf1r_api'
require_rel '../lib/call_histroy_exporter'

history_file = ARGV[0]
# output_file = ARGV[1]

api = Sf1rApi.new("180.153.140.110")
in_file = File.open(history_file, "r")
# out_file = File.new(output_file, "w")
exporter = CallHistroyExporter.new(api)
in_file.each_with_index do |line, i|
  
  $stderr.puts "Processing line #{i}" if i%10==0
  line.strip!
  next if line.empty?
  request = JSON.parse(line)
  collection = request['collection']
  next if collection.nil?
  next if collection!='b5ma'
  #$stderr.puts request['collection']
  crequest = exporter.convert(request)
  next if crequest==nil
  if crequest.class == [].class
    crequest.each do |c|
#       out_file.puts c.to_json
      puts c.to_json
    end
  else
#     out_file.puts crequest.to_json
    puts crequest.to_json
  end
  
end
in_file.close
# out_file.close
