#!/usr/bin/env ruby

require 'rubygems'
require 'rest_client'
require 'json'
require 'yaml'
require 'set'

require_relative "job"

class LexiconJob < Job
  
  def run
    read_file()
    @@collections.each do |collection|
      request = source_request_template
      request[:collection] = collection
      request[:group].each do |item|
        item[:property] = @property
      end
      response = Job.send_request(request)
      get_labels(response)
    end
    update_file()
  end

  protected

  def read_file
    @hash = Hash.new
    @hash.clear()
    if false == File.exist?(@file_path)
      return
    end
    file = File.open(@file_path, "r")
    while(line = file.gets)
      line.chomp!()
      name = line.split(":")[0]
      @hash.store(name,name)
    end
  end

  def update_file
    file = File.open(@file_path, 'a')

    @toAppend.each do |item|
       file.puts(item)
    end
    @toAppend.clear()
  end

  def get_labels(response)
    @toAppend = Array.new
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
