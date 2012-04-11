#!/usr/bin/env ruby

require 'rubygems'
require 'yaml'
require 'fileutils'
require_relative 'b5m_helper'


top_dir = File.dirname(File.expand_path(__FILE__))
config = nil
if !ARGV.empty?
  config_file = ARGV[0]
  if File.exist?(config_file)
    config = YAML::load( File.open(config_file) )
  end
else
  config_file = File.join(top_dir, "config.yml")
  default_config_file = File.join(top_dir, "config.yml.default")
  if File.exist?(config_file)
    config = YAML::load(File.open(config_file))
  elsif File.exist?(default_config_file)
    config = YAML::load(File.open(default_config_file))
  end
end
if config.nil?
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
db_path = File.join(work_dir, 'db')
odb = File.join(db_path, 'odb')
pdb = File.join(db_path, 'pdb')
mdb = File.join(db_path, 'mdb')
tmp_dir = match_path['tmp']
result_file = match_path['result']
train_scd = match_path['train_scd']
scd = match_path['scd']
comment_scd = match_path['comment_scd']
cma = match_path['cma']
b5mo_scd = File.join(match_path['b5mo'], "scd", "index")
b5mp_scd = File.join(match_path['b5mp'], "scd", "index")
b5mc_scd = File.join(match_path['b5mc'], "scd", "index")
[b5mo_scd, b5mp_scd, b5mc_scd].each do |scd_path|
  FileUtils.mkdir_p(scd_path) unless File.exist?(scd_path)
end
matcher_program = File.join(top_dir, "b5m_matcher")
FileUtils.rm_rf("#{match_path['b5m_scd']}")
Dir.mkdir(work_dir) unless File.exist?(work_dir)
Dir.mkdir(db_path) unless File.exist?(db_path)
Dir.mkdir(mdb) unless File.exist?(mdb)
time_str = Time.now.strftime("%Y%m%d%H%M%S")
mdb_instance = File.join(mdb, time_str)
if File.exist?(mdb_instance)
  exit(-1)
else
  Dir.mkdir(mdb_instance)
end
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
  a_scd = File.join(category_dir, "A")
  t_scd = File.join(category_dir, "T")
  FileUtils.rm_rf(a_scd) if File.exist?(a_scd)
  FileUtils.rm_rf(t_scd) if File.exist?(t_scd)

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

#split SCDs
system("#{matcher_program} -P -N T -S #{train_scd} -K #{work_dir}")
system("#{matcher_program} -P -N A -S #{scd} -K #{work_dir}")

task_list.each do |task|
  next unless task.valid
  category_dir = File.join(work_dir, task.cid)
  #run c++ matcher program
  category_a_scd = File.join(category_dir, "A.SCD")
  match_file = File.join(category_dir, "match")
  FileUtils.rm_rf(match_file) if File.exist?(match_file)

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

end

#merge match file to mdb_instance
system("cat #{work_dir}/C*/match > #{mdb_instance}/match")

#b5mo generator
system("#{matcher_program} --b5mo-generate -S #{scd} --b5mo #{b5mo_scd} -K #{mdb_instance} --odb #{odb}")

#uue generator
system("#{matcher_program} --uue-generate --b5mo #{b5mo_scd} --uue #{mdb_instance}/uue --odb #{odb}")

#b5mp generator, update odb and pdb here.
system("#{matcher_program} --b5mp-generate --b5mo #{b5mo_scd} --b5mp #{b5mp_scd} --uue #{mdb_instance}/uue --odb #{odb} --pdb #{pdb}")

#b5mc generator
system("#{matcher_program} --b5mc-generate --b5mc #{b5mc_scd} -S #{comment_scd} --odb #{odb}")

#logserver update
system("#{matcher_program} --logserver-update --logserver-config '#{log_server}' --uue #{mdb_instance}/uue --odb #{odb}")

