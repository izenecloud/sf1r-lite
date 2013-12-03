#!/usr/bin/env ruby

require 'rubygems'
require 'rest_client'
require 'json'
require 'yaml'
require 'set'


require_relative "job"

class TrieJob < Job
  def run
    @@collections.each do |collection|
      request = request_template
      request[:collection] = collection
      request[:group].each do |item|
        item[:property] = @property
      end
      response = Job.send_request(request)
      update_labels(response)
    end
  end

  protected

  def update_labels(response)
    group = response["group"]
    group.each do |item|
      labels = item["labels"]
      file = File.open(@file_path, 'w')
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
