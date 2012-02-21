#!/usr/bin/ruby -w
# parse the cluster-agent log file such as "cc.log",
# extract and output json request

if ARGV.size < 1 || (ARGV[0] == "-h" || ARGV[0] == "--help")
  puts "usage: #{$0} cc.log >json.txt"
  exit 1
end

PREFIX = "INFO  com.izenesoft.agent.plugin.sf1.Sf1Client  - Send JSON request: "

count = 0
while line = ARGF.gets
  if line =~ /#{PREFIX}(.+) At: /
    puts $~[1]

    count += 1
    $stderr.print "\rjson count #{count}" if count % 1000 == 0
  end
end

$stderr.puts "\rjson count #{count}"
