require 'rubygems'
require 'require_all'
require 'uri'
require_rel '../../lib/sf1r_api'
require_rel '../../lib/scd_parser'
require_rel '../../lib/scd_writer'

m_scd_file = ARGV[0]

parser = ScdParser.new(m_scd_file)
newegg_file = File.new("./newegg.SCD", "w")

_360buy_file = File.new("./360buy.SCD", "w")

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
  if host.end_with? "newegg.com.cn"
    newegg_file.puts "<Url>#{url}"
    newegg_file.puts "<DOCID>#{docid}"
#     puts url
  end
  if host.end_with? "360buy.com"
    _360buy_file.puts "<Url>#{url}"
    _360buy_file.puts "<DOCID>#{docid}"
  end
end
newegg_file.close
_360buy_file.close
