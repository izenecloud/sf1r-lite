#!/usr/bin/env ruby
# read json from input file, and send request to Log Server Driver

require 'rubygems'
require 'require_all'
require 'yaml'
require 'json'
require 'set'
require 'sf1-driver'
require 'sf1-util/scd_parser'

# Sf1r to be updated, pass as command parameter
# beta: 10.10.1.112, stage: 10.10.1.111
$Sf1rHost = "172.16.0.162" 
$Sf1rPort = 18181

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
    top_dir = File.dirname(__FILE__)
    config_file = File.join(top_dir, "config.yml")
    unless File.exist? config_file
      config_file = File.join(top_dir, "config.yml.default")
    end

    config = YAML::load_file config_file

    @conn = Sf1Driver.new(config["ba"]["ip"], config["ba"]["port"])
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
    if action == "update_cclog" or action == "convert_raw_cclog" or action == "backup_raw_cclog"
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
      "host" => "#{$Sf1rHost}",
      "port" => "#{$Sf1rPort}",
      "filename" => "#{filename}",
      "record" => request
    }
  end
 
  def sendCclogFile(filename, action="update_cclog", extract=false)
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
          if line =~ /Send JSON request: (\{.+\})/
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
    
    sendFlushRequest(filename, action)
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
    
    sendFlushRequest(filename, action)
  end
  
  def sendFlushRequest(filename, action)    
    #flush_request = wrapLogServerRequest(action, filename, nil)
    flush_request = {
      "header" => {
        "controller" => "log_server",
        "action" => "flush"
      },
      "filename" => "#{filename}",
      "action" => "#{action}"
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
      "host" => "#{$Sf1rHost}",
      "port" => "#{$Sf1rPort}"
    }

    response = @conn.call("log_server/update_documents", request)
    if response["error"]
       $stderr.puts "update_documents error: #{response["error"]}"
    end
    puts "result: #{response.to_json}"
  end  

  def send(cmd, filename)
    if cmd == "update_cclog"
      sendCclogFile(filename, "update_cclog", true)
    elsif cmd == "update_cclog_history"
      countFile = 0
      Dir.foreach(filename) do |file|                
        if file =~ /.*log$/
          countFile += 1
          log_file = "#{filename}/#{file}"   
          puts "--> send file: #{log_file}"       
          sendCclogFile(log_file, "update_cclog")
        end
      end
      puts "\nSent #{countFile} history cclog files (in form of *.log)."
    elsif cmd == "backup_raw_cclog"
      sendCclogFile(filename, "backup_raw_cclog")
    elsif cmd == "convert_raw_cclog"
      sendCclogFile(filename, "convert_raw_cclog")
    elsif cmd == "scd"
      sendScdFile(filename)
    elsif cmd == "flush"
      sendFlushRequest(filename, "all")
    elsif cmd == "update_comments"
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
    puts "Usage: #{$0} <command> <parameter> [destSf1Host] [destSf1Port]"
    puts ""
    puts "commands:  (configure SF1R address at the header of this script)"
    puts "  update_cclog <cclog_file>          : update cclog (with uuids) to configured SF1R."
    puts "  update_cclog_history <cclog_dir>   : update cclog histories."
    puts "                                       for cclog, destSf1 supposed to be Stage(10.10.1.111)"
    puts "  backup_raw_cclog  <cclog_file>     : backup cclog with uuids to cclog with raw docids." 
    puts "  convert_raw_cclog <raw_cclog_file> : convert cclog with raw docids to cclog with latest uuids."
    puts "  update_comments <collection>       : update newly logged comments(docuemts) to configured SF1R,"
    puts "                                       for comments, destSf1 supposed to be WWW(10.10.1.110)"
    puts ""
    exit(1)
  end
  
  # Log Server ip address is set at "../libdriver/config.yml.default"
  # example:
  # ------------------
  # ba:
  #   ip: 172.16.0.161
  #   port: 18812
  # ------------------

  cmd = ARGV[0]
  param = ARGV[1]

  if cmd == "update_cclog" or cmd == "update_cclog_history" or cmd == "update_comments"
    if ARGV.size < 3
      puts "[destSf1Host] required!"
      puts "Usage: #{$0} <command> <parameter> [destSf1Host] [destSf1Port]"
      exit(1)
    end
    $Sf1rHost = ARGV[2]

    if ARGV.size >= 4
      $Sf1rPort = ARGV[3]
    end
  end  

  sender = LogServerJsonSender.new()
  sender.send(cmd, param)
end
