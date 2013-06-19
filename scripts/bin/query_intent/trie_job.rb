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

class TrieJob
  CONFIG_FILE = "lexicon_update_config.yml"
  REST_TIMEOUT = 10
  REST_OPEN_TIMEOUT = 10

  def initialize(config, property)
      @config = config
      host = @config["sf1"]
      ip = host["ip"]
      port = host["port"]
      @uri_prefix = "http://#{ip}:#{port}/sf1r"
      @property = property
      directory =@config["directory"]
      directory += "/"
      @file_path = directory + property
      file = File.open(@file_path, 'w')
      file.close()
      @collections = @config["collection"].to_set
  end

  def run
    @collections.each do |collection|
      request = request_template
      request[:collection] = collection
      #request[:group][:property] = @property
      response = send_request(request)
      update_labels(response)
    end
  end

  protected

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

  def update_labels(response)
    group = response["group"]
    group.each do |item|
      labels = item["labels"]
      file = File.open(@file_path, 'a')
      labels.each do |label|
        s =  label["label"]
        file.puts s
        file.puts "["
        sub_labels = label["sub_labels"]
        if nil != sub_labels
          sub_labels.each do |sub_label|
            sub = "    "
            sub += sub_label["label"]
            file.puts sub
            file.puts "    ["
            sub_sub_labels = sub_label["sub_labels"]
            if nil != sub_sub_labels
              sub_sub_labels.each do|sub_sub_label|
                sub_sub = "        "
                sub_sub += sub_sub_label["label"]
                file.puts sub_sub
              end
            end
            file.puts "    ]"
          end
        end
        file.puts "]"
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

  def request_template 
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
          :property=>"TargetCategory"
        }
      ]
    }
  end

end
