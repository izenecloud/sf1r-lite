#!/usr/bin/env ruby
#
require 'rubygems'
require 'msgpack'
require 'msgpack/rpc'

client = MessagePack::RPC::Client.new('172.16.0.168', 18821)
localfile = 'test_imgrpc.rb'
data = ''
open(localfile) { |f|
    data = f.read
}

#msg = [false, 0, "~/workspace/sf1/sf1r-engine/source/process/ImageServerProcess/Image-f7b55a91e5d431983b8d3f179687806a.pic"];
if data.length > 0
    msg = [false, 1, data];
    rsp = client.call(:upload_image, msg)
    p rsp
end

#p client.call(:compute_image_color, [false, "./success_upload_tfs", 1]);

#p client.call( :delete_image, [false, rsp[2] ])

#p client.call(:export_image, ['~/workspace/sf1/sf1r-engine/bin/image_upload_log', '~/workspace/sf1/sf1r-engine/bin/imgout'])

