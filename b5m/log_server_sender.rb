#!/usr/bin/env ruby

require 'rubygems'
require 'yaml'
require 'fileutils'

if ARGV.size<1
  puts "Please specify scd path"
  exit(false)
end
scd_path = ARGV[0]
use_config_scd = false
if ARGV[0] == "-F"
  use_config_scd = true
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
work_dir = match_path['work_dir']
tmp_dir = match_path['tmp']
scd = match_path['scd']
b5mo_scd = File.join(match_path['b5mo'], "scd", "index")
b5mp_scd = File.join(match_path['b5mp'], "scd", "index")
if use_config_scd
  scd_path = b5mo_scd
end

matcher_program = File.join(top_dir, "b5m_matcher")
cmd = "#{matcher_program} -L '#{log_server}' -S #{scd_path} -K #{work_dir}"
puts cmd
system(cmd)

