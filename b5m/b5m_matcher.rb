#!/usr/bin/env ruby

require 'rubygems'
require 'yaml'
require 'fileutils'

top_dir = File.dirname(File.expand_path(__FILE__))
config_file = File.join(top_dir, "config.yml")
config = YAML::load(File.open(config_file))
match_config = config["matcher"]
Dir.chdir(File.dirname(config_file)) do
  match_config['path_of'].each_pair do |k,v|
    match_config['path_of'][k] = File.expand_path(v)
  end
end
match_path = match_config['path_of']
synonym = match_path['synonym']
category_file = match_path['category']
filter_attrib_name = match_path['filter_attrib_name']
work_dir = match_path['work_dir']
result_file = match_path['result']
train_scd = match_path['train_scd']
scd = match_path['scd']
cma = match_path['cma']
matcher_program = File.join(top_dir, "b5m_matcher")
Dir.mkdir(work_dir) unless File.exist?(work_dir)
IO.readlines(category_file).each do |c_line|
  c_line.strip!
  next if c_line.empty?
  next if c_line.start_with?("#")
  a = c_line.split(',')
  category_regex = a[0]
  cid = a[1]
  category_dir = File.join(work_dir, cid)
  unless File.exist?(category_dir)
    Dir.mkdir(category_dir)
    regex_file = File.join(category_dir, "category")
    ofs = File.open(regex_file, 'w')
    ofs.puts(category_regex)
    ofs.close
    fan_file = File.join(category_dir, "filter_attrib_name")
    FileUtils.cp(filter_attrib_name,fan_file)
    #run c++ matcher program
    puts "start building attribute index for #{cid}"
    system("#{matcher_program} -A -Y #{synonym} -C #{cma} -S #{train_scd} -K #{category_dir}")
    puts "start matching for #{cid}"
    output = File.join(category_dir, "match")
    FileUtils.rm_rf(output) if File.exist?(output)
    system("#{matcher_program} -B -Y #{synonym} -C #{cma} -S #{scd} -K #{category_dir} -O #{output}")
    system("cat #{output} >> #{result_file}")
  else
    puts "#{cid} exists, skip"
  end
end

