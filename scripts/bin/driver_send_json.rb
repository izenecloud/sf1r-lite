#!/usr/bin/ruby -w
# read json from input file, and send request to SF1 Driver or Log Server Driver

require 'rubygems'
require 'require_all'
require 'json'
require 'set'
require_rel '../libdriver/common'

if ARGV.size > 0 && (ARGV[0] == "-h" || ARGV[0] == "--help")
  puts "usage: #{$0} json.txt"
  exit 1
end

HOST = "172.16.0.36"
PORT = "18812"
URL_PREFIX = "http://#{HOST}:#{PORT}/apps/sf1r/instances/_/api"
FILTER = Set["documents/log_group_label",
             "documents/visit",
             "recommend/add_user",
             "recommend/update_user",
             "recommend/remove_user",
             "recommend/visit_item",
             "recommend/purchase_item",
             "recommend/track_event",
             "recommend/rate_item"]

ERROR_LOG = "errors.log"
errorFile = File.open(ERROR_LOG, "a")
totalCount = 0
successCount = 0
failCount = 0

conn = create_connection

while line = ARGF.gets
  line = line.chomp!
  next if line.empty?

  request = JSON.parse(line)

  header = request["header"]
  next if header.nil?

  controller = header["controller"]
  action = header["action"]
  next if controller.nil?

  uri = "#{controller}/#{action}"
  next unless FILTER.include? uri

#  url = "#{URL_PREFIX}/#{uri}"
#  result = RestClient.post(url, request.to_json, :content_type => :json)
#  response = JSON.parse(result)
  conn.batch do end # nothing (avoid warning)
  response = conn.call(uri, request)

  if response
    if response["header"] && response["header"]["success"]
      successCount += 1
    else
      failCount += 1
      errorFile.puts "request: #{line}"
      errorFile.puts "errors: #{response["errors"]}"
    end
  else
    failCount += 1
  end

  totalCount += 1
  $stderr.print "\r#{successCount} success, #{failCount} fail in #{ERROR_LOG}" if totalCount % 10 == 0
end

errorFile.close
$stderr.puts "\r#{successCount} success, #{failCount} fail in #{ERROR_LOG}"
