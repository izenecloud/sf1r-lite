require 'rubygems'
require 'detail/sf1r_api'
require 'detail/call_histroy_exporter'

history_file = ARGV[0]
# output_file = ARGV[1]

api = Sf1rApi.new("172.16.0.165")
in_file = File.open(history_file, "r")
# out_file = File.new(output_file, "w")
exporter = CallHistroyExporter.new(api)
in_file.each_with_index do |line, i|
  
  $stderr.puts "Processing line #{i}" if i%10==0
  line.strip!
  next if line.empty?
  request = JSON.parse(line)
  crequest = exporter.convert(request)
  next if crequest==nil
  if crequest.class == [].class
    crequest.each do |c|
#       out_file.puts c.to_json
      puts c.to_json
    end
  else
#     out_file.puts crequest.to_json
    puts crequest.to_json
  end
  
end
in_file.close
# out_file.close
