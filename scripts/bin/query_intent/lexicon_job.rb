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

class LexiconJob
  CONFIG_FILE = "lexicon_update_config.yml"
  REST_TIMEOUT = 10
  REST_OPEN_TIMEOUT = 10

  def initialize
      @config = YAML::load_file CONFIG_FILE
      host = @config["sf1"]
      ip = host["ip"]
      port = host["port"]
      @uri_prefix = "http://#{ip}:#{port}/sf1r"
      @resource =@config["resource"]
      @collections = @config["collection"].to_set
      @hash = Hash.new
      @toAppend = Array.new
  end

  def run
    read_file()
    @collections.each do |collection|
      request = source_request_template
      request[:collection] = collection
      response = send_request(request)
      get_labels(response)
    end
    update_file()
    @collections.each do |collection|
      request = query_intent_request_template
      request[:collection] = collection
      response = send_request(request)
      if response["header"] && response["header"]["success"]
        $stdout.print "Reload lexicon successfully for collection: #{collection}\n"
      end
    end
  end

  protected

  def read_file
    @hash.clear()
    file = File.open(@resource, "r")
    while(line = file.gets)
      line.chomp!()
      name = line.split(":")[0]
      @hash.store(name,name)
    end
  end

  def update_file
    file = File.open(@resource, 'a')

    @toAppend.each do |item|
       file.puts(item)
    end
    @toAppend.clear()
  end

  def send_request(request)
    uri = get_uri(request)
    RestClient.open_timeout = REST_OPEN_TIMEOUT
    RestClient.timeout = REST_TIMEOUT
    result = RestClient.post(uri, request.to_json, :content_type => :json)
    @response = JSON.parse(result)    
  end

  def get_uri(request)
    header = request[:header]
    return nil if header.nil?

    collection = request[:collection]
    controller = header[:controller]
    action = header[:action]
    @uri = "#{@uri_prefix}/#{controller}/#{action}"
  end

  def get_labels(response)
    group = response["group"];
    group.each do |item|
        labels = item["labels"]
        labels.each do |label|
            la = label["label"]
            if false == @hash.has_key?(la)
                @toAppend.push(la)
            end
        end
    end
  end
  def query_intent_request_template
    @request = {
      :header=> {
        :controller=> "query_intent",
        :action=> "reload"
      },
      :collection=> ""
   }
  end

  def source_request_template 
    @request = {
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
  end

end
