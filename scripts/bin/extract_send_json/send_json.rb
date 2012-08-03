#!/usr/bin/env ruby
# read json from input file, and send request to SF1

require 'rubygems'
require 'rest_client'
require 'json'
require 'yaml'
require 'set'

CONFIG_FILE = "send_config.yml"
unless File.exists? CONFIG_FILE
  $stderr.puts "error: require config file \"#{CONFIG_FILE}\""
  exit 1
end

config = YAML::load_file CONFIG_FILE

if ARGV.size < 1 || ARGV[0] == "-h" || ARGV[0] == "--help"
  $stderr.puts "usage: #{$0} [json.txt]..."
  exit 2
end

host = config["sf1"]
IP = host["ip"]
PORT = host["port"]
URL_PREFIX = "http://#{IP}:#{PORT}/sf1r"

$uriSet = nil
if config["uri"] && !config["uri"].empty?
  $uriSet = config["uri"].to_set
end

$collectionSet = nil
if config["collection"] && !config["collection"].empty?
  $collectionSet = config["collection"].to_set
end

def validURI?(uri)
  $uriSet.nil? || $uriSet.include?(uri)
end

def validCollection?(collection)
  $collectionSet.nil? || $collectionSet.include?(collection)
end

def getURI(request)
  header = request["header"]
  return nil if header.nil?

  collection = request["collection"]
  controller = header["controller"]
  action = header["action"]
  uri = "#{controller}/#{action}"

  if controller && validURI?(uri) && validCollection?(collection)
    uri
  else
    nil
  end
end

ERROR_LOG = "send_json.log"
errorFile = File.open(ERROR_LOG, "a")
totalCount = 0
successCount = 0
failCount = 0

while line = ARGF.gets
  line = line.chomp!
  next if line.empty?

  request = JSON.parse(line) rescue next
  uri = getURI(request)
  next if uri.nil?

  url = "#{URL_PREFIX}/#{uri}"
  result = RestClient.post(url, request.to_json, :content_type => :json)
  response = JSON.parse(result)

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
