#!/usr/bin/env ruby

require 'rubygems'
require 'yaml'
require 'fileutils'


force = false;
if ARGV.size>0 and ARGV[0]=="-F"
  force = true
  puts "FORCE mode"
end

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
valid_categories = []
all_categories = []
IO.readlines(category_file).each do |c_line|
  c_line.strip!
  next if c_line.empty?
  commentted = false
  a = c_line.split(',')
  category_regex = a[0]
  cid = a[1]
  if category_regex.start_with?("#")
    category_regex = category_regex[1..-1]
    commentted = true;
  end
  category_regex_str = category_regex
  opposite_list = nil
  if category_regex=="OPPOSITE"
    opposite_list = valid_categories
  elsif category_regex=="OPPOSITEALL"
    opposite_list = all_categories
  else
    if !commentted
      valid_categories << category_regex
    end
    all_categories << category_regex
  end

  next if commentted

  unless opposite_list.nil?
    category_regex_str = ""
    bfirst = true
    opposite_list.each do |oc|
      category_regex_str += "\n" unless bfirst
      category_regex_str += oc
      bfirst = false
    end
    category_regex_str += "\n"
  end

  category_dir = File.join(work_dir, cid)
  Dir.mkdir(category_dir) unless File.exist?(category_dir)

  index_done = File.join(category_dir, "index.done")
  match_done = File.join(category_dir, "match.done")
  if force
    FileUtils.rm_rf(index_done) if File.exist?(index_done)
    FileUtils.rm_rf(match_done) if File.exist?(match_done)
  end

  puts "#{category_regex_str}"
  regex_file = File.join(category_dir, "category")
  ofs = File.open(regex_file, 'w')
  ofs.puts(category_regex_str)
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

