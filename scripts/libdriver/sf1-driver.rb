#--
# Author::  Ian Yang
# Created:: <2010-07-13 15:10:37>
#++
# See Sf1Driver

require "sf1-driver/helper"
require "sf1-driver/raw-client"

require 'pp'

# Driver client
class Sf1Driver
  class ServerError < RuntimeError
  end

  VERSION = "2.0.1"

  # Max sequence number. It is also the upper limit of the number of requests
  # in a batch request.
  #
  # max(int32) - 1
  MAX_SEQUENCE = (1 << 31) - 2

  # Tells the sequence number of next request
  attr_reader :sequence

  # save error for batch sending
  attr_reader :server_error

  # Alias of new
  def self.open(host, port, opts = {}, &block) #:yields: self
    Sf1Driver.new(host, port, opts, &block)
  end

  # Connect server listening on host:port
  #
  # Parameters:
  #
  # [host] IP of the host BA is running on.
  # [port] Port BA is listening.
  # [opts] Options
  #        [format] Select underlying writer and reader, now only "json"
  #                 is supported, and it is also the default format.
  def initialize(host, port, opts = {}) #:yields: self
    opts = {:format => "json"}.merge opts

    use_format(opts[:format])

    @raw_client = opts[:client] || RawClient.new(host, port)

    # 0 is reserved by server
    @sequence = 1

    if block_given?
      yield self
      close
    end
  end

  # Closes the connection
  def close
    @raw_client.close
  end

  # Chooses data format. Now only "json" is supported, and it is also the default format.
  def use_format(format)
    reader_file = File.join(File.dirname(__FILE__), "sf1-driver", "readers", "#{format}-reader")
    writer_file = File.join(File.dirname(__FILE__), "sf1-driver", "writers", "#{format}-writer")
    require reader_file
    require writer_file

    eval "extend #{Helper.camel(format)}Reader"
    eval "extend #{Helper.camel(format)}Writer"
  end

  # Send request.
  #
  # Parameters:
  #
  # [+uri+] a string with format "/controller/action". If action is "index",
  #         it can be omitted
  # [+request+] just a hash
  #
  # Return:
  #
  # * When used in non batch mode, this function is synchronous. The request
  #   is sent to function and wait for the response. After then, the response
  #   is used as the return value of this function.
  #
  # * When send is used in batch block, the request is sent after the block is
  #   closed. The allocated sequence number is returned immediately. Responses
  #   are returned in function block.
  #
  # Examples:
  #
  #     sf1 = Sf1Driver.new(host, port)
  #     request = {
  #       :collection => "ChnWiki",
  #       :resource => {
  #         :DOCID => 1
  #         :title => "SF1v5 Driver Howto"
  #       }
  #     }
  #     sf1.call("documents/create", request)
  #
  def call(uri, request, tokens = "")
    raise "Two many requests in batch" if @batch_requests and @batch_requests.length == MAX_SEQUENCE
    remember_sequence = @sequence

    request = stringize_header_keys(request)
    if request["header"]["check_time"]
      start_time = Time.now
    end

    tokens ||= ""
    if @batch_requests
      send_request(uri, request, tokens)
      @batch_requests << start_time
      remember_sequence
    else
      response_sequence, response = send_request_and_get_response(uri, request, tokens)

      raise ServerError, "Unmatch sequence number" unless response_sequence == remember_sequence

      if start_time
        response["timers"] ||= {}
        response["timers"]["total_client_time"] = Time.now - start_time
      end

      response
    end
  end

  # Deprecated. Use call instead.
  def send(*args)
    puts "Warning: Sf1Driver#send is deprecated, use Sf1Driver#call instead"

    call(*args)
  end

  # Open a block that call can used to add requests in batch.
  #
  # In batch block, call returns sequence number of that request
  # immediately. Responses are returned in this function.
  #
  # e.g.
  #
  # sf1 = Sf1Driver.new(host, port)
  # responses = sf1.batch do
  #   sf1.call "/ChnWiki/commands/create", :resource => {:command => "index"}
  #   sf1.call "/ChnWiki/commands/create", :resource => {:command => "mining"}
  # end
  #
  def batch
    raise "Cannot nest batch" if @batch_requests
    @batch_requests = []
    @server_error = nil
    responses = []

    begin
      yield self
      unless @batch_requests.empty?
        request_count = @batch_requests.length
        responses = Array.new(request_count)
        if @sequence <= request_count
          begin_index = MAX_SEQUENCE - (request_count - @sequence)
        else
          begin_index = @sequence - request_count
        end

        request_count.times do
          response_sequence, response = get_response
          if response_sequence < begin_index
            index = MAX_SEQUENCE - begin_index + response_sequence
          else
            index = response_sequence - begin_index
          end
          raise ServerError, "Unmatch sequence number" unless index < request_count

          if @batch_requests[index]
            response["timers"] ||= {}
            response["timers"]["total_client_time"] = Time.now - @batch_requests[index]
          end

          responses[index] = response
        end
      end
    rescue ServerError => e
      @server_error = e
      @raw_client.close
    rescue
      @raw_client.close
      raise
    ensure
      @batch_requests = nil
    end

    responses
  end

private
  def auto_connect
    begin
      yield
    rescue Errno::ECONNRESET, Errno::EPIPE, Errno::ECONNABORTED, Errno::ETIMEDOUT
      if @client.reconnect
        yield
      else
        raise Errno::ECONNRESET
      end
    end
  end

  def send_request_and_get_response(uri, request, tokens)
    request = parse_uri(uri, request, tokens)

    response = nil
    begin
      auto_connect do
        @raw_client.send_request(@sequence, writer_serialize(request))
        response = get_response
      end
    ensure
      increase_sequence
    end

    response
  end

  # Send request to server
  def send_request(uri, request, tokens)
    begin
      request = parse_uri(uri, request, tokens)
      @raw_client.send_request(@sequence, writer_serialize(request))
    ensure
      increase_sequence
    end
  end

  # Read response from server
  def get_response
    response = @raw_client.get_response
    if response
      response[1] = reader_deserialize(response.last)
    else
      raise ServerError, "No response."
    end

    raise ServerError, "Malformed response." unless response[1].is_a?Hash

    if response[0] == 0
      response[1]["errors"] ||= ["Unknown server error."]
      raise ServerError, response[1]["errors"].join("\n")
    end

    response
  end

  def increase_sequence
    @sequence += 1
    if @sequence > MAX_SEQUENCE
      @sequence = 1
    end
  end

  def parse_uri(uri, request, tokens)
    controller, action = uri.to_s.split("/").reject{|e| e.nil? || e.empty?}

    if controller.nil?
      raise ArgumentError, "Require controller name."
    end

    request["header"]["controller"] = controller
    request["header"]["action"] = action if action
    request["header"]["acl_tokens"] = tokens

    request
  end
  
  def stringize_header_keys(request)
    header = request["header"] || request[:header] || {}
    request.delete :header
    if header.is_a? Hash
      header = header.inject({}) do |h, (key, value)|
        h[key.to_s] = value
        h
      end
    end

    request["header"] = header
    pp request
    request
  end
end
