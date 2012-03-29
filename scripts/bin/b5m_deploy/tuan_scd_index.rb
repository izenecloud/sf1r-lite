require 'rubygems'
require 'require_all'
require 'sf1-driver'

conn = Sf1Driver.new("180.153.140.112", "18181")
body = {}
body["collection"] = "tuanm"
response = conn.call("commands/index", body)
puts response
