#!/usr/bin/env ruby
require 'rubygems'
require 'rufus/scheduler'

require_relative "lexicon_job"

scheduler = Rufus::Scheduler.start_new

lexicon_job = LexiconJob.new
scheduler.cron '00 02 * * 1-7' do
  lexicon_job.run
end

scheduler.join

