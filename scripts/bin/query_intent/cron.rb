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

lexicon_job = LexiconJob.new(config, "Source")
trie_job = TrieJob.new(config, "TargetCategory")
scheduler = Rufus::Scheduler.start_new

scheduler.cron '00 02 * * 1-7' do
  trie_job.run
  lexicon_job.run
end

scheduler.join
