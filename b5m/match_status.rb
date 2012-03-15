#!/usr/bin/env ruby

require 'rubygems'
require 'yaml'
require 'fileutils'
require_relative 'b5m_helper'

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
category_file = match_path['category']

name_list = []
Dir.foreach(work_dir) do |w|
  next unless w.start_with?("C")
  name_list << w
end

name_list.sort!

disable_set = {}

name_list.each do |w|
  category_dir = File.join(work_dir, w)
  match_file = File.join(category_dir, "match")
  if File.exist?(match_file)
    #STDOUT.puts("#{w} HAS_MATCH")
  else
    #STDOUT.puts("#{w} NO_MATCH")
    disable_set[w] = true
  end
end

task_list = []
IO.readlines(category_file).each do |c_line|
  c_line.strip!
  task = CategoryTask.new(c_line)
  task.info['disable'] = true unless disable_set[task.cid].nil?
  task_list << task
end

task_list.each do |t|
  STDOUT.puts t
end

