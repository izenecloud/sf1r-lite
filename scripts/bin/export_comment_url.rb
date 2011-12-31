require 'rubygems'
require 'require_all'
require 'uri'
require_rel '../lib/scd_parser'
require_rel '../lib/scd_writer'
require_rel '../lib/sf1r_api.rb'

m_scd_file = ARGV[0]

parser = ScdParser.new(m_scd_file)

dangdang_file = File.new("./dangdang.SCD", "w")
amazon_file = File.new("./amazon.SCD", "w")

count = 0
parser = ScdParser.new(m_scd_file)
parser.each do |doc|
  count += 1
  puts "doc count #{count}" if count%10000==0
  docid = doc['DOCID']
  next if docid.nil?
  category = doc['Category']
  next if category.nil?
  category.strip!
  next if category.size==0
  next if category.start_with?("图书")
  #puts "Category #{category}"
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
    dangdang_file.puts "<Url>#{url}"
    dangdang_file.puts "<DOCID>#{docid}"
#     puts url
  end
  if host.end_with? "amazon.cn"
    amazon_file.puts "<url>#{url}"
    amazon_file.puts "<DOCID>#{docid}"
  end
end
dangdang_file.close
amazon_file.close
