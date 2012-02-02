#!/usr/bin/env ruby

require 'rubygems'
require 'require_all'
require_rel '../lib/sf1r_api'
require_rel '../lib/call_histroy_exporter'

#ip = "180.153.140.110"
ip = ARGV[0]
cc_file = ARGV[1]
PREFIX = "INFO  com.izenesoft.agent.plugin.sf1.Sf1Client  - Send JSON request: "
api = Sf1rApi.new(ip)
in_file = File.open(cc_file, "r")
exporter = CallHistroyExporter.new(api)
in_file.each_with_index do |line, i|
  
  $stderr.puts "Processing line #{i}" if i%10==0
  line.strip!
  if line =~ /#{PREFIX}(.+) At: /
    line = $~[1]
  else
    line = ""
  end
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
      puts c.to_json
    end
  else
    puts crequest.to_json
  end
  
end
in_file.close

