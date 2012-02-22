#!/usr/bin/ruby
# read json from input file, and send request to Log Server Driver

require 'rubygems'
require 'require_all'
require 'json'
require 'set'
require_rel '../libdriver/common'
require_rel '../lib/scd_parser'
        
Sf1rHost = "172.16.0.162" # to be specified
Sf1rPort = 18181

PREFIX = "INFO  com.izenesoft.agent.plugin.sf1.Sf1Client  - Send JSON request: "

class LogServerJsonSender
  def initialize(host=nil, port=nil)
    @host = host
    @port = port        
    @filter_map = {}
    @filter_map['documents/log_group_label'] = true
    @filter_map['recommend/add_user'] = true
    @filter_map['recommend/update_user'] = true
    @filter_map['recommend/remove_user'] = true
    @filter_map['documents/visit'] = true
    @filter_map['recommend/visit_item'] = true
    @filter_map['recommend/purchase_item'] = true
    @conn = create_connection
  end
  
  def filterCclogRequest(request)
    return true if request.nil?
      
    header = request["header"]
    return true if header.nil?
      
    controller = header["controller"]
    action = header["action"]
    return true if controller.nil?
    return true if action.nil?
    
    uri = "#{controller}/#{action}"
    filter = @filter_map[uri]    
    return true if filter.nil?
      
    return false
  end
  
  def wrapLogServerRequest(action, filename, record)
    if action == "update_cclog" or action == "convert_raw_cclog"
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
 
  def sendCclogFile(filename, action="update_cclog", extract=true)
    file = File.new(filename, "r")
    errorFileName = "#{filename}.errors"
    errorFile = File.open(errorFileName, "w")
    
    totalCount = 0
    successCount = 0
    failCount = 0    

    # send update requests
    while line = file.gets
        line = line.chomp!
        next if line.empty?
        
        # extract logged request from cclog
        if extract == true
          if line =~ /#{PREFIX}(.+) At: /
            line = $~[1]
          else
            next
          end
        end
        
        # warp request
        request = wrapLogServerRequest(action, filename, line)
        
        # filter unnecessary requests
        next if filterCclogRequest(request["record"])        
        
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
            $stderr.print  "errors: #{response["errors"]}"
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
    errorFileName = "#{filename}.errors"
    errorFile = File.open(errorFileName, "w")
    
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

  def updateComments(collection)
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
    if cmd == "update_cclog"
      sendCclogFile(filename, "update_cclog")
    elsif cmd == "update_cclog_history"
      Dir.foreach(filename) do |file|        
        puts file
        if file =~ /.*log/
          log_file = "#{filename}/#{file}"
          puts log_file 
          sendCclogFile(log_file, "update_cclog", false)
        end
      end
    elsif cmd == "convert_raw_cclog"
      sendCclogFile(filename, "convert_raw_cclog")
    elsif cmd == "scd"
      sendScdFile(filename)
    elsif cmd == "flush"
      sendFlushRequest(filename)
    elsif cmd == "comments"
      collection = filename
      updateComments(collection)
    else
      puts "Unrecognized command: #{cmd}"
      exit(1)
    end
  end  
end

if __FILE__ == $0  
  if ARGV.size < 2 || (ARGV[0] == "-h" || ARGV[0] == "--help")
    puts "Log Server Client:"
    puts "  Set log server address at \"../libdriver/config.yml.default\" "
    puts ""
    puts "Usage: #{$0} <command> <parameter>"
    puts ""
    puts "commands:  (configure SF1R address at the header of this script)"
    puts "  update_cclog <cclog_file>          : update cclog (with uuids) to configured SF1R."
    puts "  update_cclog_history <cclog_dir>   : update cclog histories."
    puts "  convert_raw_cclog <raw_cclog_file> : convert cclog with raw docids to cclog with latest uuids."
    puts "  comments <collection>              : update newly logged comments(docuemts) to configured SF1R."
    puts ""
    exit(1)
  end
  
  # Set IP address of Log Server Driver at "../libdriver/config.yml.default"
  # example:
  # ------------------
  # ba:
  #   ip: 172.16.0.161
  #   port: 18812
  # ------------------
  sender = LogServerJsonSender.new()
  sender.send(ARGV[0], ARGV[1])
end
