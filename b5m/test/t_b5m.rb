require 'sf1-util/scd_writer'
require 'sf1-util/scd_parser'
require 'b5m/mock_b5m'
require 'fileutils'
require_relative '../b5m_helper'

describe "B5M Tester" do

  def prepare_empty_dir(path)
    FileUtils.rm_rf(path) if File.exist?(path)
    FileUtils.mkdir_p(path) unless File.exist?(path)
  end

  before do
    @top_dir = File.dirname(__FILE__)
    @config_file = File.join(@top_dir, "config.yml")
    @config = YAML::load(File.open(config_file))
    match_config = @config["matcher"]
    @log_server = match_config["log_server"]
    Dir.chdir(File.dirname(@config_file)) do
      match_config['path_of'].each_pair do |k,v|
        match_config['path_of'][k] = File.expand_path(v)
      end
    end
    match_path = match_config['path_of']
    @synonym = match_path['synonym']
    @category_file = match_path['category']
    @filter_attrib_name = match_path['filter_attrib_name']
    @work_dir = match_path['work_dir']
    @db_path = File.join(work_dir, 'db')
    @odb = File.join(db_path, 'odb')
    @pdb = File.join(db_path, 'pdb')
    @mdb = File.join(db_path, 'mdb')
    @tmp_dir = match_path['tmp']
    @result_file = match_path['result']
    @train_scd = match_path['train_scd']
    @scd = match_path['scd']
    @comment_scd = match_path['comment_scd']
    @cma = match_path['cma']
    @b5mo_scd = File.join(match_path['b5mo'], "scd", "index")
    @b5mp_scd = File.join(match_path['b5mp'], "scd", "index")
    @b5mc_scd = File.join(match_path['b5mc'], "scd", "index")
    [@b5mo_scd, @b5mp_scd, @b5mc_scd].each do |scd_path|
      prepare_empty_dir(scd_path)
    end
    Dir.mkdir(@work_dir) unless File.exist?(@work_dir)
    Dir.mkdir(@db_path) unless File.exist?(@db_path)
    Dir.mkdir(@mdb) unless File.exist?(@mdb)
    @matcher = B5MPath.matcher
  end

  it "should always be right" do
    #parameters
    iteration = 500
    reindex_prob = 0.05

    #begin test, given b5mo scd, to test b5mp and log server
    mock_b5m = MockB5M.new
    reindex = true
    while iteration>0
      iteration-=1
      if reindex
        FileUtils.rm_rf(work_dir) if File.exist?(work_dir)
      end
      #prepare_empty_dir(@scd)
      #time_str = Time.now.strftime("%Y%m%d%H%M%S")
      mdb_instance = File.join(@mdb, "test_instance")
      prepare_empty_dir(mdb_instance)
      prepare_empty_dir(@b5mo_scd)

      insert_docs = []
      update_docs = []
      delete_docs = []

      unless insert_docs.empty?
        writer = ScdWriter.new(@b5mo_scd, "I")
        insert_docs.each do |doc|
          writer.append(doc)
        end
      end
      unless update_docs.empty?
        writer = ScdWriter.new(@b5mo_scd, "U")
        update_docs.each do |doc|
          writer.append(doc)
        end
      end
      unless delete_docs.empty?
        writer = ScdWriter.new(@b5mo_scd, "D")
        delete_docs.each do |doc|
          writer.append(doc)
        end
      end

      #reset reindex
      ran = rand()
      if ran<reindex_prob
        reindex = true
      else
        reindex = false
      end
    end
  end
end

