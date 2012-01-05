#!/usr/bin/ruby -w
# 1. parse the cluster-agent log file such as "cc.log"
# 2. extract API "documents()", which parameter [search][log_group_labels] is true
# 3. convert to API "log_group_label()"
# 4. output new cluster-agent log containing API "log_group_label()"
require 'rubygems'
require 'require_all'
require_rel 'group_log_parser'

if ARGV.size > 0 && (ARGV[0] == "-h" || ARGV[0] == "--help")
  puts "usage: #{$0} cc.log >group_label.cc.log"
  exit 1
end

PREFIX = "INFO  com.izenesoft.agent.plugin.sf1.Sf1Client  - Send JSON request: "
parser = GroupLogParser.new
requestCount = 0
convertCount = 0

while line = ARGF.gets
  line = line.chomp!

  if line =~ /^(.+#{PREFIX})(.+)( At: .+)$/
    prefix = $~[1]
    suffix = $~[3]

    new_request = parser.parse($~[2])
    if new_request
      puts "#{prefix}#{new_request}#{suffix}"
      convertCount += 1
    end

    requestCount += 1
    $stderr.print "\rrequests: #{requestCount}, converted: #{convertCount}" if requestCount % 1000 == 0
  end
end

$stderr.puts "\rrequests: #{requestCount}, converted: #{convertCount}"
