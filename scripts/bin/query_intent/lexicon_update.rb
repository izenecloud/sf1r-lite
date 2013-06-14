#!/usr/bin/env ruby

require 'rubygems'
require 'rest_client'
require 'json'
require 'yaml'
require 'set'

module RestClient

  class << self
    attr_accessor :timeout
    attr_accessor :open_timeout
  end

  def self.post(url, payload, headers={}, &block)
    Request.execute(:method => :post,
                    :url => url,
                    :payload => payload,
                    :headers => headers,
                    :timeout=>@timeout,
                    :open_timeout=>@open_timeout,
                    &block)
  end

end


#configure phase
CONFIG_FILE = "lexicon_update_config.yml"
unless File.exists? CONFIG_FILE
  $stderr.puts "error: require config file \"#{CONFIG_FILE}\""
  exit 1
end

config = YAML::load_file CONFIG_FILE

host = config["sf1"]
IP = host["ip"]
PORT = host["port"]
URL_PREFIX = "http://#{IP}:#{PORT}/sf1r"

$collectionSet = nil
if config["collection"] && !config["collection"].empty?
  $collectionSet = config["collection"].to_set
end

$resource = config["resource"]
$hash = Hash.new
file = File.open($resource, "r")
while(line = file.gets)
    line.chomp!()
    name = line.split(":")[0]
    $hash.store(name,name)
end

def getURI(request)
  header = request[:header]
  return nil if header.nil?

  collection = request[:collection]
  controller = header[:controller]
  action = header[:action]
  uri = "#{controller}/#{action}"
end

@query_intent_request_template = {
  :header=> {
    :controller=> "query_intent",
    :action=> "reload"
  },
  :collection=> ""
}

@source_request_template = {
  :header=> {
    :controller=> "documents",
    :action=> "search"
  },
  :collection=>"",
  :search=> {
    :keywords=>"*"
  },
  :group=>
  [
    {
      :property=>"Source"
    }
  ]
}


RestClient.open_timeout = 90000000
RestClient.timeout = 90000000

$toAppend = Array.new
$collectionSet.each do |collection|
    request = @source_request_template
    request[:collection] = collection
    uri = getURI(request)
    url = "#{URL_PREFIX}/#{uri}"
    result = RestClient.post(url, request.to_json, :content_type => :json)
    response = JSON.parse(result)
    group = response["group"];
    group.each do |item|
        labels = item["labels"]
        labels.each do |label|
            la = label["label"]
            if false == $hash.has_key?(la) then
                $toAppend.push(la)
            end
        end
    end
end

file = File.open($resource, 'a')

$toAppend.each do |item|
    file.puts(item)
end


$collectionSet.each do |collection|
    request = @query_intent_request_template 
    request[:collection] = collection
    uri = getURI(request)
    url = "#{URL_PREFIX}/#{uri}"
    result = RestClient.post(url, request.to_json, :content_type => :json)
    response = JSON.parse(result)
    if response["header"] && response["header"]["success"]
        $stdout.print "Reload lexicon successfully for collection: #{collection}\n"
    end
end
