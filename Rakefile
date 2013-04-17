# Wrapper of CMAKE
[
 ENV["EXTRA_CMAKE_MODULES_DIRS"],
 File.join(File.dirname(__FILE__), "../cmake"), # in same top directory
 File.join(File.dirname(__FILE__), "../../cmake--master/workspace") # for hudson
].each do |dir|
  next unless dir
  dir = File.expand_path(dir)
  if File.exists? File.join(dir, "Findizenelib.cmake")
    $: << File.join(dir, "lib")
    ENV["EXTRA_CMAKE_MODULES_DIRS"] = dir
  end
end

require "izenesoft/project-finder"
# Must find default dependent projects before require izenesoft/tasks,
# which will load env.yml or cmake.yml and override the values found here.
finder = IZENESOFT::ProjectFinder.new(File.dirname(__FILE__))
finder.find_izenelib
finder.find_ilplib
finder.find_imllib
#finder.find_iise
finder.find_kma
finder.find_icma
finder.find_ijma
finder.find_idmlib

require "izenesoft/tasks"

task :default => :cmake

#task :install do
  #require 'fileutils'
  #begin
    #top_dir = File.dirname(File.expand_path(__FILE__))
    #tools = [File.join(top_dir, "tool", "ScdMergeTool")]
    #gem_home = ENV['GEM_HOME']
    #abort 'GEM_HOME not found' if gem_home.nil?
    #dest_dir = File.join(gem_home, "bin")
    #STDERR.puts "dest dir #{dest_dir}"
    #tools.each do |tool|
      #dest = File.join(dest_dir, File.basename(tool))
      #STDERR.puts "installing #{dest}"
      #FileUtils.cp tool, dest
    #end
  #rescue Exception => e
    #STDERR.puts "install err #{e}"
  #end
#end

IZENESOFT::CMake.new

IZENESOFT::BoostTest.new
