#!/usr/bin/ruby
# read json from input file, and send request to Log Server Driver

require 'rubygems'
require 'require_all'
require 'json'
require 'set'
require_rel '../libdriver/common'

FILTER = Set["documents/log_group_label",
             "documents/visit",
             "recommend/add_user",
             "recommend/update_user",
             "recommend/remove_user",
             "recommend/visit_item",
             "recommend/purchase_item",
             "recommend/track_event",
             "recommend/rate_item"]
             
ERROR_LOG = "errors.log"

class LogServerJsonSender
  def initialize(host, port)
    @host = host
    @port = port
    
    @conn = create_connection
  end
  
  def send(cmd, filename)
    if cmd == "cclog"
      sendCclogFile filename
    elsif cmd == "scd"
      sendScdFile filename
    else
      puts "Unrecognized command: #{cmd}"
    end
  end
  
  def sendCclogFile(filename)
    file = File.new(filename, "r")
    errorFile = File.open(ERROR_LOG, "a")
    
    totalCount = 0
    successCount = 0
    failCount = 0    

    while line = file.gets
        line = line.chomp!
        next if line.empty?
        
        request = JSON.parse(line)
        
        header = request["header"]
        next if header.nil?
      
        controller = header["controller"]
        action = header["action"]
        next if controller.nil?
        
        uri = "#{controller}/#{action}"
        next unless FILTER.include? uri
        
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
        $stderr.print "\r#{successCount} success, #{failCount} fail in #{ERROR_LOG}" if totalCount % 10 == 0
    end
    
    errorFile.close
    $stderr.puts "\r#{successCount} success, #{failCount} fail in #{ERROR_LOG}"
  end
  
  def sendScdFile(file)
    
  end
  
end

if __FILE__ == $0  
  if ARGV.size < 2 || (ARGV[0] == "-h" || ARGV[0] == "--help")
    puts "Usage: #{$0} {cclog|scd} {filename}"
    exit(1)
  end
  
  sender = LogServerJsonSender.new("172.16.0.36", 18812)
  sender.send(ARGV[0], ARGV[1])
end