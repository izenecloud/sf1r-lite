require 'scd_parser'
require 'scd_writer'
require 'detail/sf1r_api.rb'
require 'uri'
a_scd_file = ARGV[0]
m_scd_file = ARGV[1]
map_scd_file = ARGV[2]

# def get_all_doc(api, mname, uuid)
#   controller = "documents"
#   action = "get"
#   body = {}
#   body['collection'] = mname
#   body['conditions'] = []
#   condition = {}
#   condition['property'] = 'uuid'
#   condition['operator'] = '='
#   condition['value'] = []
#   condition['value'][0] = uuid
#   body['conditions'][0] = condition
#   body['select'] = []
#   selec = {}
#   selec['property'] = "DOCID"
#   body['select'][0] = selec
#   selec2 = {}
#   selec2['property'] = "Url"
#   body['select'][1] = selec2
#   response = api.send(controller, action, body)
#   return nil if response['header'].nil?
#   return nil if !response['header']['success']
#   return nil if response['resources'].nil?
#   doc_list = []
#   response['resources'].each do |r|
#     doc_list << r
#   end
# #   puts "#{uuid} : #{doc_list}"
#   return doc_list
# end
# 
# api = Sf1rApi.new("172.16.0.165")
parser = ScdParser.new(a_scd_file)
uuid_map = {}
count = 0
parser.each do |doc|
  count += 1
  puts "doc count #{count}" if count%10000==0
  itemcount = doc['itemcount']
  next if itemcount.nil?
  ii = itemcount.to_i
  next if ii<2
  
  uuid = doc['DOCID']
  uuid_map[uuid] = 1
end

puts "Find #{uuid_map.size} uuids"

docid_map = {}

parser = ScdParser.new(map_scd_file)
count = 0
parser.each do |doc|
  count += 1
  puts "doc count #{count}" if count%10000==0
  uuid = doc['uuid']
  next if uuid.nil?
  next if !uuid_map.has_key? uuid
  docid = doc['DOCID']
  docid_map[docid] = 1
end

docid_file = File.new("./docid_map", "w")

docid_map.each do |docid, v|
  docid_file.puts docid
end
docid_file.close

puts "Find #{docid_map.size} DOCIDs"

dangdang_file = File.new("./dangdang.SCD", "w")
amazon_file = File.new("./amazon.SCD", "w")

count = 0
parser = ScdParser.new(m_scd_file)
parser.each do |doc|
  count += 1
  puts "doc count #{count}" if count%10000==0
  docid = doc['DOCID']
  next if docid.nil?
  
  url = doc['Url']
  next if url.nil?
  uri = nil
  begin
    uri = URI.parse url
  rescue
    next
  end
  host = uri.host
  if host.end_with? "dangdang.com"
    next if !docid_map.has_key? docid
    dangdang_file.puts "<Url>#{url}"
    dangdang_file.puts "<DOCID>#{docid}"
#     puts url
  end
  if host.end_with? "amazon.cn"
    next if !docid_map.has_key? docid
    amazon_file.puts "<url>#{url}"
    amazon_file.puts "<DOCID>#{docid}"
  end
end
dangdang_file.close
amazon_file.close
