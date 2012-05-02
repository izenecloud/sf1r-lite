#!/usr/bin/env ruby

require 'rubygems'
require 'yaml'
require 'fileutils'
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
doreindex = false
domerge = true
dob5mc = true
dologserver = true
ARGV.each_with_index do |a,i|
  if a=="--reindex"
    doreindex = true
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
odb = File.join(db_path, 'odb')
pdb = File.join(db_path, 'pdb')
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
FileUtils.rm_rf(work_dir) if doreindex
FileUtils.mkdir_p(work_dir) unless File.exist?(work_dir)
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

reindex = mdb_instance_list.empty?()?true:false

time_str = Time.now.strftime("%Y%m%d%H%M%S")
mdb_instance = File.join(mdb, time_str)
merged_scd = File.join(mdb_instance, "merged")
raw_scd = File.join(mdb_instance, "raw")
comment_merged_scd = File.join(mdb_instance, "comment_merged")
if File.exist?(mdb_instance)
  abort "#{mdb_instance} already exists"
else
  Dir.mkdir(mdb_instance)
  Dir.mkdir(merged_scd)
  Dir.mkdir(raw_scd)
  Dir.mkdir(comment_merged_scd)
end
task_list = []

require_relative 'b5m_env.rb'

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
  #FileUtils.rm_rf(t_scd) if File.exist?(t_scd)

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

#do scd merge
if domerge
  if reindex
    puts "reindex!!"
    system("#{merger_program} -I #{scd} -O #{merged_scd}")
    abort("merge scd failed") unless $?.success?
  else
    system("#{merger_program} -I #{scd} -O #{merged_scd} --gen-all")
    abort("merge scd failed") unless $?.success?
  end
else
  system("cp #{scd}/*.SCD #{merged_scd}/")
end

#generate raw scds
system("#{matcher_program} --raw-generate -S #{merged_scd} --raw #{raw_scd} --odb #{odb}")
abort("generating raw scd failed") unless $?.success?

#split SCDs
system("#{matcher_program} -P -N T -S #{train_scd} -K #{work_dir}")
abort("spliting T scd failed") unless $?.success?
system("#{matcher_program} -P -N A -S #{raw_scd} -K #{work_dir}")
abort("spliting A scd failed") unless $?.success?

task_list.each do |task|
  next unless task.valid
  category_dir = File.join(work_dir, task.cid)
  #run c++ matcher program
  index_done = File.join(category_dir, "index.done")
  match_done = File.join(category_dir, "match.done")
  match_file = File.join(category_dir, "match")
  FileUtils.rm_rf(match_file) if File.exist?(match_file)
  FileUtils.rm_rf(match_done) if File.exist?(match_done)
  #FileUtils.rm_rf(index_done) if File.exist?(index_done)

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
    system("#{matcher_program} -I -C #{cma} -S #{raw_scd} -K #{category_dir} --dictionary #{simhash_dic}")
    abort("similarity matching failed") unless $?.success?
  else
    next unless task.info['disable'].nil?
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

b5mo_scd_instance = File.join(mdb_instance, "b5mo_scd")
b5mp_scd_instance = File.join(mdb_instance, "b5mp_scd")
b5mc_scd_instance = File.join(mdb_instance, "b5mc_scd")
Dir.mkdir(b5mo_scd_instance)
Dir.mkdir(b5mp_scd_instance)
Dir.mkdir(b5mc_scd_instance)


#b5mo generator
system("#{matcher_program} --b5mo-generate -S #{raw_scd} --b5mo #{b5mo_scd_instance} -K #{mdb_instance}")
abort("b5mo generate failed") unless $?.success?
system("rm -rf #{b5mo_scd}/*")
system("cp #{b5mo_scd_instance}/* #{b5mo_scd}/")

#uue generator
system("#{matcher_program} --uue-generate --b5mo #{b5mo_scd_instance} --uue #{mdb_instance}/uue --odb #{odb}")
abort("uue generate failed") unless $?.success?

#b5mp generator, update odb and pdb here.
system("#{matcher_program} --b5mp-generate --b5mo #{b5mo_scd_instance} --b5mp #{b5mp_scd_instance} --uue #{mdb_instance}/uue --odb #{odb} --pdb #{pdb}")
system("rm -rf #{b5mp_scd}/*")
system("cp #{b5mp_scd_instance}/* #{b5mp_scd}/")
odb_in_mdb = File.join(mdb_instance, "odb")
pdb_in_mdb = File.join(mdb_instance, "pdb")
FileUtils.cp_r(odb, odb_in_mdb)
FileUtils.cp_r(pdb, pdb_in_mdb)

#merge comment scd
if dob5mc
  if domerge
    system("#{merger_program} -I #{comment_scd} -O #{comment_merged_scd}")
    abort("merge comment scd failed") unless $?.success?
  else
    system("cp #{comment_scd}/*.SCD #{comment_merged_scd}/")
  end

  #b5mc generator
  cmd = "#{matcher_program} --b5mc-generate --b5mc #{b5mc_scd_instance} -S #{comment_merged_scd} --odb #{odb} --uue #{mdb_instance}/uue"
  unless last_mdb_instance.nil?
    last_b5mc_scd = File.join(last_mdb_instance, "b5mc_scd")
    cmd += " --old-scd-path #{last_b5mc_scd}"
  end
  puts cmd
  system(cmd)
  abort("b5mc generate failed") unless $?.success?
  system("rm -rf #{b5mc_scd}/*")
  system("cp #{b5mc_scd_instance}/* #{b5mc_scd}/")
  unless last_mdb_instance.nil?
    last_b5mc_scd = File.join(last_mdb_instance, "b5mc_scd")
    system("cp #{last_b5mc_scd}/* #{b5mc_scd_instance}/")
    b5mc_merging = File.join(mdb_instance, "b5mc_merging")
    Dir.mkdir b5mc_merging
    system("#{merger_program} -I #{b5mc_scd_instance} -O #{b5mc_merging}")
    FileUtils.rm_rf(b5mc_scd_instance)
    FileUtils.mv(b5mc_merging, b5mc_scd_instance)
  end
end

if dologserver
  #logserver update
  system("#{matcher_program} --logserver-update --logserver-config '#{log_server}' --uue #{mdb_instance}/uue --odb #{odb}")
  abort("log server update failed") unless $?.success?
end

