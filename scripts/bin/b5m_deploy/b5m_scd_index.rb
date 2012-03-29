require 'rubygems'
require 'require_all'
require 'sf1-driver'

conn = Sf1Driver.new("180.153.140.112", "18181")
body = {}
body["collection"] = "b5mp"
response = conn.call("commands/index", body)
puts response
body["collection"] = "b5mo"
response = conn.call("commands/index", body)
puts response
body["collection"] = "b5mc"
response = conn.call("commands/index", body)
puts response
