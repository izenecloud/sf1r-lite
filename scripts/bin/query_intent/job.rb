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

class Job
  REST_TIMEOUT = 10
  REST_OPEN_TIMEOUT = 10

  def initialize(config, property)
      @@config = YAML::load_file CONFIG_FILE
      host = @@config["nginx"]
      ip = host["ip"]
      port = host["port"]
      @@uri_prefix = "http://#{ip}:#{port}/sf1r"
      @@collections = @@config["collection"].to_set
      @@directory = @@config["directory"]
      @property = property
      @file_path = @@directory + "/" + @property
  end

  def run
  end

  def self.update()
    @@collections.each do |collection|
      request = query_intent_request_template
      request[:collection] = collection
      response = send_request(request)
      if response["header"] && response["header"]["success"]
        $stdout.print "Reload lexicon successfully for collection: #{collection}\n"
      end
    end
  end

  protected

  def self.send_request(request)
    uri = get_uri(request)
    RestClient.open_timeout = REST_OPEN_TIMEOUT
    RestClient.timeout = REST_TIMEOUT
    result = RestClient.post(uri, request.to_json, :content_type => :json)
    @@response = JSON.parse(result)    
  end

  def self.get_uri(request)
    header = request[:header]
    return nil if header.nil?

    collection = request[:collection]
    controller = header[:controller]
    action = header[:action]
    @@uri = "#{@@uri_prefix}/#{controller}/#{action}"
  end

  def self.query_intent_request_template
    @@request = {
      :header=> {
        :controller=> "query_intent",
        :action=> "reload"
      },
      :collection=> ""
   }
  end

end
