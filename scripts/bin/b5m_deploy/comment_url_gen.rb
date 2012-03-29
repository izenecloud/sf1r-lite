require 'rubygems'
require 'require_all'
require 'uri'
require 'sf1-util/scd_parser'
require 'sf1-util/scd_writer'

m_scd_file = ARGV[0]

parser = ScdParser.new(m_scd_file)
amazon_file = File.new("./amazon.SCD", "w")
the360buy_file = File.new("./360buy.SCD", "w")
newegg_file = File.new("./newegg.SCD", "w")
dangdang_file = File.new("./dangdang.SCD", "w")

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
  if host.end_with? "amazon.cn"
    amazon_file.puts "<Url>#{url}"
    amazon_file.puts "<DOCID>#{docid}"
  end
  if host.end_with? "dangdang.com"
    dangdang_file.puts "<Url>#{url}"
    dangdang_file.puts "<DOCID>#{docid}"
  end
  if host.end_with? "360buy.com"
    the360buy_file.puts "<Url>#{url}"
    the360buy_file.puts "<DOCID>#{docid}"
  end
  if host.end_with? "newegg.com"
    newegg_file.puts "<Url>#{url}"
    newegg_file.puts "<DOCID>#{docid}"
  end
end
amazon_file.close
dangdang_file.close
the360buy_file.close
newegg_file.close
