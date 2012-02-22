#!/usr/bin/env ruby

require 'rubygems'
require 'yaml'
require 'fileutils'

top_dir = File.dirname(File.expand_path(__FILE__))
config_file = File.join(top_dir, "config.yml")
default_config_file = File.join(top_dir, "config.yml.default")
if File.exist?(config_file)
  config = YAML::load(File.open(config_file))
elsif File.exist?(default_config_file)
  config = YAML::load(File.open(default_config_file))
else
  puts 'config file not found'
  exit(false)
end
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
b5mo_scd = File.join(match_path['b5mo'], "scd", "index")
b5mp_scd = File.join(match_path['b5mp'], "scd", "index")
matcher_program = File.join(top_dir, "b5m_matcher")
FileUtils.rm_rf("#{match_path['b5m_scd']}")
Dir.mkdir(work_dir) unless File.exist?(work_dir)
IO.readlines(category_file).each do |c_line|
  c_line.strip!
  next if c_line.empty?
  next if c_line.start_with?("#")
  a = c_line.split(',')
  category_regex = a[0]
  cid = a[1]
  category_dir = File.join(work_dir, cid)
  Dir.mkdir(category_dir) unless File.exist?(category_dir)
  regex_file = File.join(category_dir, "category")
  ofs = File.open(regex_file, 'w')
  ofs.puts(category_regex)
  ofs.close
  fan_file = File.join(category_dir, "filter_attrib_name")

  #FileUtils.cp(filter_attrib_name,fan_file)
  #run c++ matcher program
  category_a_scd = File.join(category_dir, "A.SCD")
  unless File.exist?(category_a_scd)
    system("#{matcher_program} -P -S #{scd} -K #{category_dir}")
  end

  if a.size>=4 and a[2]=="COMPLETE"
    puts "start complete matching #{cid}"
    attrib_file = File.join(category_dir,"attrib_name")
    ofs = File.open(attrib_file, 'w')
    ofs.puts(a[3])
    ofs.close
    system("#{matcher_program} -M -S #{scd} -K #{category_dir}")
  elsif a.size>=3 and a[2]=="SIMILARITY"
    #do similarity matching
    puts "start similarity matching #{cid}"
    system("#{matcher_program} -I -C #{cma} -S #{scd} -K #{category_dir}")
  else
    puts "start building attribute index for #{cid}"
    system("#{matcher_program} -A -Y #{synonym} -C #{cma} -S #{train_scd} -K #{category_dir}")
    match_done = File.join(category_dir, "match.done")
    unless File.exist?(match_done)
      puts "start matching for #{cid}"
      output = File.join(category_dir, "match")
      system("#{matcher_program} -B -Y #{synonym} -C #{cma} -S #{scd} -K #{category_dir}")
    end

  end

  source_scd = scd
  source_scd = category_a_scd if File.exist?(category_a_scd)
  puts "start generate b5mo and b5mp SCDs from #{source_scd}"
  system("#{matcher_program} -G #{match_path['b5m_scd']} -S #{source_scd} -K #{category_dir} -E")
end
system("rm -rf #{b5mo_scd}/*")
system("cp #{match_path['b5m_scd']}/b5mo/* #{b5mo_scd}/")
system("rm -rf #{b5mp_scd}/*")
system("cp #{match_path['b5m_scd']}/b5mp/* #{b5mp_scd}/")

