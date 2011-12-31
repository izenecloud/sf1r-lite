require 'rubygems'
require 'require_all'
require_rel '../../lib/sf1r_api'


api = Sf1rApi.new("180.153.140.112")
body = {}
body["collection"] = "b5mm"
response = api.send("commands", nil, body)
puts response
