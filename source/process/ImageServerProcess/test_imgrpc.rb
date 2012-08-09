#!/usr/bin/env ruby
#
require 'rubygems'
require 'msgpack'
require 'msgpack/rpc'

client = MessagePack::RPC::Client.new('127.0.0.1', 18821)

msg = [false, 0, "/home/vincentlee/workspace/sf1/sf1r-engine/source/process/ImageServerProcess/Image-f7b55a91e5d431983b8d3f179687806a.pic"];
rsp = client.call(:upload_image, msg)
p rsp

#p client.call( :delete_image, [false, rsp[2] ])

#p client.call(:export_image, ['/home/vincentlee/workspace/sf1/sf1r-engine/bin/image_upload_log', '/home/vincentlee/workspace/sf1/sf1r-engine/bin/imgout'])
