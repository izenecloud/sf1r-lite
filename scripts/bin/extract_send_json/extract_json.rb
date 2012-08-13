#!/usr/bin/env ruby
# parse the nginx error.log, extract and output json request

if ARGV.size < 2 || ARGV[0] == "-h" || ARGV[0] == "--help"
  puts "usage: #{$0} json.txt [error.log]..."
  exit 1
end

jsonName = ARGV.shift
jsonFile = File.open(jsonName, "w")
count = 0

while line = ARGF.gets
  if line =~ /Send JSON request: (\{.+\})/
    jsonFile.puts $~[1]

    count += 1
    $stderr.print "\rjson count #{count}" if count % 1000 == 0
  end
end

$stderr.puts "\rjson count #{count}"
