#!/usr/bin/ruby
# read json from input file, and send request to Log Server Driver

require 'rubygems'
require 'require_all'
require 'json'
require 'set'
require_rel '../libdriver/common'
        
class LogServerJsonSender
  def initialize(host, port)
    @host = host
    @port = port        
    @conn = create_connection
  end
  
  def wrapJsonRequest(cmd, filename, line)
    if cmd == "cclog" 
      action = "update_cclog"
      request = JSON.parse(line)
    elsif cmd == "scd"
      action = "update_scd"
      request = line
    elsif cmd == "flush"
      action = "flush"
      request = line
    else
      puts "Unrecognized command: #{cmd}"
      exit(1)
    end
    
    request_wrap = {
      "header" => {
        "controller" => "log_server",
        "action" => "#{action}"
      },
      "filename" => "#{filename}",
      "line" => request
    }    
  end
  
  def send(cmd, filename)
    if cmd == "cclog"
      nil
    elsif cmd == "scd"
      nil
    else
      puts "Unrecognized command: #{cmd}"
      exit(1)
    end
    
    sendFile(cmd, filename)
  end  
 
  def sendFile(cmd, filename)
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
        
        request = wrapJsonRequest(cmd, filename, line)
        
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
    
    # send flush request 
    $stderr.puts "\n--> flush:"
    flush_request = wrapJsonRequest("flush", filename, nil)
    response = @conn.call("log_server/flush", flush_request)
    if response["errors"]
      $stderr.puts "--> flush error: #{response["errors"]}"
    end
  end
end

if __FILE__ == $0  
  if ARGV.size < 2 || (ARGV[0] == "-h" || ARGV[0] == "--help")
    puts "Usage: #{$0} {cclog|scd} {filename}"
    exit(1)
  end
  
  # The ip address is not effective here, set it at "../libdriver/config.yml.default"
  sender = LogServerJsonSender.new("172.16.0.36", 18812)
  sender.send(ARGV[0], ARGV[1])
end
