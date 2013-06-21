#!/usr/bin/env ruby
require 'rubygems'
require 'rufus/scheduler'
require 'rest_client'
require 'json'
require 'yaml'
require 'set'

require_relative "lexicon_job"
require_relative "trie_job"

CONFIG_FILE = "lexicon_update_config.yml"
config = YAML::load_file CONFIG_FILE

jobs = Array.new

lexicons = config["lexicon"].to_set
lexicons.each do |property|
  lexicon_job = LexiconJob.new(config, property)
  jobs.push(lexicon_job)
end

tries = config["trie"].to_set
tries.each do |property|
  tries_job = TrieJob.new(config, property)
  jobs.push(tries_job);
end

scheduler = Rufus::Scheduler.start_new

scheduler.cron '00 02 * * 1-7' do
  jobs.each do |job|
   job.run
  end
  Job.update
end

scheduler.join
