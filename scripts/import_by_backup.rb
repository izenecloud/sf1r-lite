require 'rubygems'
require 'detail/sf1r_api'
require 'detail/backup_histroy_importer'

backup_file = ARGV[0]

api = Sf1rApi.new("172.16.0.167")
in_file = File.open(backup_file, "r")

importer = BackupHistroyImporter.new(api)
in_file.each_with_index do |line, i|
  $stderr.puts "Processing line #{i}" if i%10==0
  line.strip!
  next if line.empty?
  request = JSON.parse(line)
  importer.add_import(request)
end
in_file.close
importer.flush
