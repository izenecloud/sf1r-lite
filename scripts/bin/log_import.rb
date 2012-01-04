#!/usr/bin/env ruby

require 'rubygems'
require 'require_all'
require '../lib/sf1r_api'
require '../lib/backup_histroy_importer'

backup_file = ARGV[0]

api = Sf1rApi.new("180.153.140.109")
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
