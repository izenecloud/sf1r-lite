#!/usr/bin/ruby
# read json from input file, and send request to Log Server Driver

require 'rubygems'
require 'require_all'
require 'json'
require 'set'
require_rel '../libdriver/common'
require_rel '../lib/scd_parser'
        
Sf1rHost = "172.16.0.36"
Sf1rPort = 18181

class LogServerJsonSender
  def initialize(host, port)
    @host = host
    @port = port        
    @conn = create_connection
  end
  
  def wrapLogServerRequest(action, filename, record)
    if action == "update_cclog" 
      request = JSON.parse(record)
    elsif action == "update_scd"
      request = record
    elsif action == "flush"
      #request = record
    else
      puts "Unrecognized log server action: #{cmd}"
      exit(1)
    end
    
    request_wrap = {
      "header" => {
        "controller" => "log_server",
        "action" => "#{action}"
      },
      "host" => "#{Sf1rHost}",
      "port" => "#{Sf1rPort}",
      "filename" => "#{filename}",
      "record" => request
    }
  end
 
  def sendCclogFile(filename, action="update_cclog")
    file = File.new(filename, "r")
    errorFileName = "#{filename}.errors.log"
    errorFile = File.open(errorFileName, "a")
    
    totalCount = 0
    successCount = 0
    failCount = 0    

    # send update requests
    while line = file.gets
        line = line.chomp!
        next if line.empty?
        
        request = wrapLogServerRequest(action, filename, line)
        
        header = request["header"]
        next if header.nil?
      
        controller = header["controller"]
        action = header["action"]
        next if controller.nil?
        
        uri = "#{controller}/#{action}"
        
        @conn.batch do end # nothing (avoid warning)
        response = @conn.call(uri, request)
      
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
        $stderr.print "\r#{successCount} success, #{failCount} fail in #{errorFileName}" if totalCount % 10 == 0
    end
    
    errorFile.close
    $stderr.puts "\r#{successCount} success, #{failCount} fail in #{errorFileName}"
    
    sendFlushRequest(filename)
  end
  
  def sendScdFile(filename, action="update_scd")
    parser = ScdParser.new(filename)
    errorFileName = "#{filename}.errors.log"
    errorFile = File.open(errorFileName, "a")
    
    totalCount = 0
    successCount = 0
    failCount = 0  
    
    parser.each do |doc|
      # set <DOCID> as the first property  
      doc_record = "<DOCID>#{doc['DOCID']}\n"
      doc.each do |key, value|
        next if key == "DOCID"
        doc_record += "<#{key}>#{value}\n"
      end
      
      #request = wrapLogServerRequest(action, filename, doc_record)
      request = {
        "header" => {
          "controller" => "log_server",
          "action" => "#{action}"
        },
        "filename" => "#{filename}",
        "DOCID" => "#{doc['DOCID']}",
        "record" => doc_record
      }
      
      header = request["header"]
      next if header.nil?
    
      controller = header["controller"]
      action = header["action"]
      next if controller.nil?
      
      uri = "#{controller}/#{action}"
      
      @conn.batch do end # nothing (avoid warning)
      response = @conn.call(uri, request)

      if response
        if response["header"] && response["header"]["success"]
          successCount += 1
        else
          failCount += 1
          errorFile.puts "request: #{uri} #{doc['DOCID']}"
          errorFile.puts "errors: #{response["errors"]}"
        end
      else
        failCount += 1
      end
    
      totalCount += 1
      $stderr.print "\r#{successCount} success, #{failCount} fail in #{errorFileName}" if totalCount % 10 == 0
    end
    
    errorFile.close
    $stderr.puts "\r#{successCount} success, #{failCount} fail in #{errorFileName}"
    
    sendFlushRequest(filename)
  end
  
  def sendFlushRequest(filename, action="flush")    
    #flush_request = wrapLogServerRequest(action, filename, nil)
    flush_request = {
      "header" => {
        "controller" => "log_server",
        "action" => "#{action}"
      },
      "filename" => "#{filename}",
    }
    
    $stderr.puts "\n--> flush:"
    response = @conn.call("log_server/flush", flush_request)
    if response["errors"]
      $stderr.puts "--> flush error: #{response["errors"]}"
    end
  end

  def updateDocuments(collection)
    # TODO
    request = {
      "header" => {
        "controller" => "log_server",
        "action" => "update_documents"
      },
      "collection" => "#{collection}",
      "host" => "#{Sf1rHost}",
      "port" => "#{Sf1rPort}"
    }

    response = @conn.call("log_server/update_documents", request)
    if response["error"]
       $stderr.puts "update_documents error: #{response["error"]}"
    end
  end  

  def send(cmd, filename)
    #cmds = ["cclog", "scd", "flush"]
    if cmd == "cclog"
      sendCclogFile(filename)
    elsif cmd == "scd"
      sendScdFile(filename)
    elsif cmd == "flush"
      sendFlushRequest(filename)
    elsif cmd == "documents"
      collection = filename
      updateDocuments(collection)
    else
      puts "Unrecognized command: #{cmd}"
      exit(1)
    end
  end  
end

if __FILE__ == $0  
  if ARGV.size < 2 || (ARGV[0] == "-h" || ARGV[0] == "--help")
    puts "Usage: #{$0} {cclog|scd|documents} {filename|collection}"
    exit(1)
  end
  
  # The IP address is not effective here, set it at "../libdriver/config.yml.default"
  sender = LogServerJsonSender.new("172.16.0.36", 18812)
  sender.send(ARGV[0], ARGV[1])
end
