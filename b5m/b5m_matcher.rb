#!/usr/bin/env ruby

require 'rubygems'
require 'yaml'
require 'fileutils'
require_relative 'b5m_helper'


force = false;
forcet = false;
forcea = false;
gen_scd = true;
ARGV.each do |a|
  if a=="-F"
    force = true
  elsif a=="-FA"
    forcea = true
  elsif a=="-FT"
    forcet = true
  elsif a=="-NOGEN"
    gen_scd = false
  end
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
log_server = match_config["log_server"]
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
task_list = []

IO.readlines(category_file).each do |c_line|
  c_line.strip!
  task = CategoryTask.new(c_line)
  task_list << task
end

task_list.each do |task|

  next unless task.valid

  category_str = task.category
  ball = false
  if category_str=="OPPOSITEALL"
    ball = true
  end
  if !task.regex
    category_str = ""
    task_list.each do |t|
      next unless t.info['disable'].nil?
      if t.regex and (t.valid or ball)
        category_str += t.category
        category_str += "\n"
      end
    end
  end

  category_dir = File.join(work_dir, task.cid)
  Dir.mkdir(category_dir) unless File.exist?(category_dir)

  index_done = File.join(category_dir, "index.done")
  match_done = File.join(category_dir, "match.done")
  a_scd = File.join(category_dir, "A.SCD")
  t_scd = File.join(category_dir, "T.SCD")
  if force
    FileUtils.rm_rf(index_done) if File.exist?(index_done)
    FileUtils.rm_rf(match_done) if File.exist?(match_done)
  end

  if forcea
    FileUtils.rm_rf(a_scd) if File.exist?(a_scd)
  end

  if forcet
    FileUtils.rm_rf(t_scd) if File.exist?(t_scd)
  end

  regex_file = File.join(category_dir, "category")
  ofs = File.open(regex_file, 'w')
  ofs.puts(category_str)
  ofs.close

  type_file = File.join(category_dir, "type")
  ofs = File.open(type_file, 'w')
  if task.type == CategoryTask::COM
    ofs.puts("complete")
  elsif task.type == CategoryTask::SIM
    ofs.puts("similarity")
  else
    ofs.puts("attribute")
  end
  ofs.close
  #fan_file = File.join(category_dir, "filter_attrib_name")
  #FileUtils.cp(filter_attrib_name,fan_file)
end

if forcea
  system("#{matcher_program} -P -N A -S #{scd} -K #{work_dir}")
end

if forcet
  system("#{matcher_program} -P -N T -S #{train_scd} -K #{work_dir}")
end

task_list.each do |task|
  next unless task.valid
  category_dir = File.join(work_dir, task.cid)
  #run c++ matcher program
  category_a_scd = File.join(category_dir, "A.SCD")
  #unless File.exist?(category_a_scd)
    #system("#{matcher_program} -P -S #{scd} -K #{category_dir}")
  #end

  if task.type == CategoryTask::COM
    puts "start complete matching #{task.cid}"
    attrib_file = File.join(category_dir,"attrib_name")
    ofs = File.open(attrib_file, 'w')
    ofs.puts(task.info['name'])
    ofs.close
    system("#{matcher_program} -M -S #{scd} -K #{category_dir}")
  elsif task.type == CategoryTask::SIM
    #do similarity matching
    puts "start similarity matching #{task.cid}"
    system("#{matcher_program} -I -C #{cma} -S #{scd} -K #{category_dir}")
  else
    next unless task.info['disable'].nil?
    puts "start building attribute index for #{task.cid}"
    system("#{matcher_program} -A -Y #{synonym} -C #{cma} -S #{train_scd} -K #{category_dir}")
    puts "start matching for #{task.cid}"
    system("#{matcher_program} -B -Y #{synonym} -C #{cma} -S #{scd} -K #{category_dir}")
  end

  if gen_scd
    source_scd = scd
    source_scd = category_a_scd if File.exist?(category_a_scd)
    puts "start generate b5mo and b5mp SCDs from #{source_scd}"
    system("#{matcher_program} -G #{match_path['b5m_scd']} -S #{source_scd} -K #{category_dir} -E")
  end
end

if gen_scd
  system("rm -rf #{b5mo_scd}/*")
  system("cp #{match_path['b5m_scd']}/b5mo/* #{b5mo_scd}/")
  system("rm -rf #{b5mp_scd}/*")
  system("cp #{match_path['b5m_scd']}/b5mp/* #{b5mp_scd}/")
end

