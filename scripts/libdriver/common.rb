#---
# Author::  Ian Yang
# Created:: <2010-06-22 16:36:55>
#+++
#
# Common for all scripts
#
# 1. Setup LOADPATH
# 2. Load config file
LIBDIR = File.dirname(File.expand_path(__FILE__)).freeze

$LOAD_PATH << LIBDIR

require "sf1-driver"
require "yaml"
config_file = File.join(LIBDIR, "config.yml")
unless File.exist? config_file
  config_file = File.join(LIBDIR, "config.yml.default")
end

CONFIG = YAML::load_file config_file

def create_connection
  Sf1Driver.new(CONFIG["ba"]["ip"], CONFIG["ba"]["port"])
end
