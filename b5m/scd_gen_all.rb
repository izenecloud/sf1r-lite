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
work_dir = match_path['work_dir']
scd = match_path['scd']
b5mo_scd = File.join(match_path['b5mo'], "scd", "index")
b5mp_scd = File.join(match_path['b5mp'], "scd", "index")
matcher_program = File.join(top_dir, "b5m_matcher")
FileUtils.rm_rf("#{match_path['b5m_scd']}")
puts "start generate b5mo and b5mp SCDs from #{scd}"
system("#{matcher_program} -G #{match_path['b5m_scd']} -S #{scd} -K #{work_dir}")
system("rm -rf #{b5mo_scd}/*")
system("cp #{match_path['b5m_scd']}/b5mo/* #{b5mo_scd}/")
system("rm -rf #{b5mp_scd}/*")
system("cp #{match_path['b5m_scd']}/b5mp/* #{b5mp_scd}/")

