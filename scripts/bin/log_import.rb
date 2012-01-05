#!/usr/bin/env ruby

require 'rubygems'
require 'require_all'
require '../lib/sf1r_api'
require '../lib/backup_histroy_importer'

#ip="180.153.140.109"
ip = ARGV[0]
ex_dir = ARGV[1]
ex_files = File.join(ex_dir,"ex*.log")

api = Sf1rApi.new(ip)
Dir.glob(ex_files) do |f|
  in_file = File.open(f, "r")
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
end
