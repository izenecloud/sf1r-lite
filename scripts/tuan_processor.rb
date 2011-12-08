require 'scd_parser'
require 'scd_writer'
require 'tuan_category'

scd_file = ARGV[0]


classifer = Classifier.new
classifer.load "./tuan.owl"
parser = ScdParser.new(scd_file)


# doc_list = Hash.new
# parser.each do |doc|
#   docid = doc['DOCID']
#   if docid==nil
#     next
#   end
#   if !doc_list.has_key? docid
#     doc_list[docid] = doc
#   else
#     doc.each do |key, value|
#       doc_list[docid][key] = value
#     end
#   end
#   p docid
#   p doc_list[docid]['TimeEnd']
# end

writer = ScdWriter.new("./")
count = 0
all_count = 0
parser.each do |doc|
#   puts "DOCID: #{doc['DOCID']}"
  all_count += 1
  endtime_str = doc['TimeEnd'].strip
#   puts "endtime_str : #{endtime_str}"
  if endtime_str.length==0
    next
  end
  time = DateTime::parse(endtime_str, "%Y%m%d%H%M%S")
  now = DateTime::now
  if time<now
    next
  end
  
  str = doc['Title']
  category = classifer.get_class(str)
  doc['Category'] = category
  writer.append doc
  count += 1
end
writer.close
puts "Total doc count : #{count} - #{all_count}"
  
