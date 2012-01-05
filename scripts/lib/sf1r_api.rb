require 'rubygems'
require 'rest_client'
require 'json'

class Sf1rApi
  def initialize(host, port = 8888, suffix = 'apps/sf1r/instances/_/api')
    @host = host
    @port = port
    @url = "#{host}:#{port}/#{suffix}"
  end
  
  def send(controller, action, body, get_response = true)
    action_str = Sf1rApi::make_action_str(controller, action)
    uri = "#{@url}/#{action_str}"
#     $stderr.puts "send #{@host} , #{uri} , #{body}"
    result = nil
    begin
      result = RestClient.post(uri, body.to_json, :content_type => :json)
    rescue => e
      $stderr.puts "fail send to #{uri}, error : #{e}"
      $stderr.puts "request #{body}"
      return nil
    end
    if get_response
      begin
        response = JSON.parse(result, :max_nesting => 100)
        #$stderr.puts "response #{response}"
        return response
      rescue => e
        $stderr.puts "fail json parsing #{result}, error : #{e}"
        return nil
      end
    end
  end
  
  def send_request(request, get_response = true)
    controller, action, req = Sf1rApi::split(request)
    return nil if controller.nil?
    send(controller, action, req, get_response)
  end
  
  
  def self.split(request)
    header = request["header"]
    return nil, nil, nil if header.nil?
    controller = header["controller"]
    action = header["action"]
    return nil, nil, nil if controller.nil?
    return controller, action, request
  end
  
  def self.make_action_str(controller, action)
    if action==nil
      controller
    else
      "#{controller}/#{action}"
    end
  end
  
end
