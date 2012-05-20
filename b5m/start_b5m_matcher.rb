#!/usr/bin/env ruby

require 'rubygems'
require 'yaml'
require 'fileutils'
require 'sf1-util/scd_parser'
require_relative 'b5m_helper'


top_dir = File.dirname(File.expand_path(__FILE__))
stop_script = File.join(top_dir, "stop_b5m_matcher.rb")
log_dir = File.join(top_dir, "log")
Dir.mkdir(log_dir) unless File.exists?(log_dir)
pid_file = File.join(log_dir, "pid")
File.open(pid_file, 'w') do |f|
  f.puts(Process.pid)
end
at_exit do
  FileUtils.rm_rf(pid_file)
end

#Signal.trap("INT") do
  #cmd = "ruby #{stop_scripts}"
  #puts "calling #{cmd}"
  #system(cmd)
#end

config = nil
config_file = nil
mode = 0 #B5MMode::INC as default
domerge = true
dob5mc = true
dologserver = true
ARGV.each_with_index do |a,i|
  if a=="--mode"
    mode = ARGV[i+1].to_i
  elsif a=="--nomerge"
    domerge = false
  elsif a=="--nob5mc"
    dob5mc = false
  elsif a=="--nologserver"
    dologserver = false
  elsif a=="--config"
    config_file = ARGV[i+1]
  end
end

if config_file.nil?
  config_file = File.join(top_dir, "config.yml")
  unless File.exists?(config_file)
    config_file = File.join(top_dir, "config.yml.default")
  end
end
config_file = nil unless File.exists?(config_file)
if config_file.nil?
  abort 'config file not found'
else
  puts "Use config #{config_file}"
  config = YAML::load_file(config_file)
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
status_path = File.join(work_dir, "status")
#odb = File.join(db_path, 'odb')
#pdb = File.join(db_path, 'pdb')
mdb = File.join(db_path, 'mdb')
tmp_dir = match_path['tmp']
result_file = match_path['result']
train_scd = match_path['train_scd']
scd = match_path['scd']
comment_scd = match_path['comment_scd']
cma = match_path['cma']
simhash_dic = match_path['simhash_dic']
b5mo_scd = File.join(match_path['b5mo'], "scd", "index")
b5mp_scd = File.join(match_path['b5mp'], "scd", "index")
b5mc_scd = File.join(match_path['b5mc'], "scd", "index")
[b5mo_scd, b5mp_scd, b5mc_scd].each do |scd_path|
  FileUtils.mkdir_p(scd_path) unless File.exist?(scd_path)
end
matcher_program = File.join(top_dir, "b5m_matcher")
merger_program = B5MPath.scd_merger
FileUtils.rm_rf("#{match_path['b5m_scd']}")
if mode==3 #retrain
  FileUtils.rm_rf(work_dir)
elsif mode==2 #rematch
  FileUtils.rm_rf(db_path)
elsif mode==1 #rebuild
end
FileUtils.mkdir_p(work_dir) unless File.exist?(work_dir)
Dir.mkdir(status_path) unless File.exist?(status_path)
Dir.mkdir(db_path) unless File.exist?(db_path)
Dir.mkdir(mdb) unless File.exist?(mdb)
mdb_instance_list = []
Dir.foreach(mdb) do |m|
  next unless m =~ /\d{14}/
  m = File.join(mdb, m)
  next unless File.directory?(m)
  mdb_instance_list << m
end

mdb_instance_list.sort!
last_mdb_instance = mdb_instance_list.last
last_odb = nil
unless last_mdb_instance.nil?
  last_odb = File.join(last_mdb_instance, "odb")
end
#reindex = mdb_instance_list.empty?()?true:false
#retrain = false

##check if train_scd modified.
#train_scd_list = ScdParser.get_scd_list(train_scd)
#if train_scd_list.size!=1
  #abort "train_scd number error : #{train_scd_list.size}"
#end

#saved_train_scd_status = nil
#train_scd_status_file = File.join(status_path, "train_scd")
#saved_train_scd_status = YAML::load_file(train_scd_status_file) if File.exist?(train_scd_status_file)
#train_scd_file = train_scd_list.first
#train_scd_status = {}
#train_scd_status['mtime'] = File.mtime(train_scd_file).strftime("%Y-%m-%d %H:%M:%S")
#train_scd_status['size'] = File.new(train_scd_file).size
#if !saved_train_scd_status.nil? and saved_train_scd_status==train_scd_status
  #retrain = false
#else
  #retrain = true
#end

#if retrain
  #puts "re-train!!"
  #File.open(train_scd_status_file, 'w') do |out|
    #YAML.dump(train_scd_status, out)
  #end
  #system("rm -rf #{work_dir}/C*/T")
#end


time_str = Time.now.strftime("%Y%m%d%H%M%S")
mdb_instance = File.join(mdb, time_str)
odb = File.join(mdb_instance, "odb")
if File.exist?(mdb_instance)
  abort "#{mdb_instance} already exists"
else
  Dir.mkdir(mdb_instance)
  unless last_odb.nil?
    FileUtils.cp_r(last_odb, odb)
  end
  mode_file = File.join(mdb_instance, "mode")
  File.open(mode_file, 'w') do |f|
    f.puts mode
  end
end
task_list = []

#require_relative 'b5m_env.rb'

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

  a_scd = File.join(category_dir, "A")
  t_scd = File.join(category_dir, "T")
  FileUtils.rm_rf(a_scd) if File.exist?(a_scd)

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
end

#generate raw scds
system("#{matcher_program} --raw-generate -S #{scd} --mdb-instance #{mdb_instance} --odb #{odb} --mode #{mode}")
abort("generating raw scd failed") unless $?.success?

#split SCDs
raw_scd = File.join(mdb_instance, "raw")
system("#{matcher_program} -P -N T -S #{train_scd} -K #{work_dir}")
abort("spliting T scd failed") unless $?.success?
system("#{matcher_program} -P -N A -S #{raw_scd} -K #{work_dir}")
abort("spliting A scd failed") unless $?.success?

task_list.each do |task|
  next unless task.valid
  category_dir = File.join(work_dir, task.cid)
  #run c++ matcher program
  match_done = File.join(category_dir, "match.done")
  match_file = File.join(category_dir, "match")
  FileUtils.rm_rf(match_file) if File.exist?(match_file)
  FileUtils.rm_rf(match_done) if File.exist?(match_done)
  a_scd = File.join(category_dir, "A")
  a_scd_list = ScdParser::get_scd_list(a_scd)
  if a_scd_list.empty? and task.type==CategoryTask::ATT
    puts "#{a_scd} empty in attribute indexing, ignore matching"
    next
  end

  if task.type == CategoryTask::COM
    puts "start complete matching #{task.cid}"
    attrib_file = File.join(category_dir,"attrib_name")
    ofs = File.open(attrib_file, 'w')
    ofs.puts(task.info['name'])
    ofs.close
    system("#{matcher_program} -M -S #{raw_scd} -K #{category_dir}")
    abort("complete matching failed") unless $?.success?
  elsif task.type == CategoryTask::SIM
    #do similarity matching
    puts "start similarity matching #{task.cid}"
    work_files = File.join(category_dir, "work_dir")
    FileUtils.rm_rf(work_files) if mode>1 and File.exist?(work_files)
    system("#{matcher_program} -I -C #{cma} -S #{raw_scd} -K #{category_dir} --dictionary #{simhash_dic}")
    abort("similarity matching failed") unless $?.success?
  else
    next unless task.info['disable'].nil?
    #index_done = File.join(category_dir, "index.done")
    #FileUtils.rm_rf(index_done) if retrain and File.exist?(index_done)
    puts "start building attribute index for #{task.cid}"
    system("#{matcher_program} -A -Y #{synonym} -C #{cma} -S #{train_scd} -K #{category_dir}")
    abort("attribute indexing failed") unless $?.success?
    puts "start matching for #{task.cid}"
    system("#{matcher_program} -B -Y #{synonym} -C #{cma} -S #{raw_scd} -K #{category_dir}")
    abort("attribute matching failed") unless $?.success?
  end

end

#merge match file to mdb_instance
system("cat #{work_dir}/C*/match > #{mdb_instance}/match")

b5mo_scd_instance = File.join(mdb_instance, "b5mo")
b5mo_mirror_instance = File.join(mdb_instance, "b5mo_mirror")
b5mp_scd_instance = File.join(mdb_instance, "b5mp")
b5mc_scd_instance = File.join(mdb_instance, "b5mc")

#b5mo generator, update odb here
system("#{matcher_program} --b5mo-generate --odb #{odb} --mdb-instance #{mdb_instance}")
abort("b5mo generate failed") unless $?.success?
system("rm -rf #{b5mo_scd}/*.SCD")
system("cp #{b5mo_scd_instance}/*.SCD #{b5mo_scd}/")

#uue generator
#system("#{matcher_program} --uue-generate --mdb-instance #{mdb_instance} --odb #{odb}")
#abort("uue generate failed") unless $?.success?

#b5mp generator

cmd = "#{matcher_program} --b5mp-generate --mdb-instance #{mdb_instance}"
if !last_mdb_instance.nil? and mode!=1
  cmd+=" --last-mdb-instance #{last_mdb_instance}"
end
system(cmd)
system("rm -rf #{b5mp_scd}/*.SCD")
system("cp #{b5mp_scd_instance}/*.SCD #{b5mp_scd}/")

#merge comment scd
if dob5mc

  #b5mc generator
  cmd = "#{matcher_program} --b5mc-generate -S #{comment_scd} --odb #{odb} --mdb-instance #{mdb_instance}"
  puts cmd
  system(cmd)
  abort("b5mc generate failed") unless $?.success?
  system("rm -rf #{b5mc_scd}/*.SCD")
  system("cp #{b5mc_scd_instance}/*.SCD #{b5mc_scd}/")
end

if dologserver
  #logserver update
  system("#{matcher_program} --logserver-update --logserver-config '#{log_server}' --mdb-instance #{mdb_instance}")
  abort("log server update failed") unless $?.success?
end

