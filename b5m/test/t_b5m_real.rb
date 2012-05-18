require 'fileutils'
require 'sf1-util/scd_writer'
require 'sf1-util/scd_parser'
require 'sf1-util/mock_dm'
require 'yaml'
require 'logger'
require_relative '../b5m_helper'
require_relative 'mock_b5m'
require_relative 'matcher/dm_matcher'
require_relative 'matcher/dmc_matcher'

describe "B5M Real Tester" do

  def config_file

    File.join(@top_dir, "real_test_config.yml")
  end

  def gen_yml_config(params)
    Dir.chdir(File.dirname(@default_config_file)) do
      params.each_pair do |k,v|
        @config["matcher"]['path_of'][k] = File.expand_path(v)
      end
    end
    File.open(config_file,'w') do |f|
      f.write(@config.to_yaml)
    end
  end

  def work_dir

    @config['matcher']['path_of']['work_dir']
  end
  
  def mdb_dir

    File.join(work_dir, "db", "mdb")
  end

  def b5mc_output_dir

    File.join(@config['matcher']['path_of']['b5mc'], "scd", "index")
  end

  def prepare_empty_dir(path)
    FileUtils.rm_rf(path) if File.exist?(path)
    FileUtils.mkdir_p(path) unless File.exist?(path)
  end

  def is_test_pid(pid)
    return pid.start_with?("40")
  end

  def puts(s)
    @logger.info s
  end

  before do
    @top_dir = File.dirname(__FILE__)
    @default_config_file = File.join(@top_dir, "config.yml")
    @config = YAML::load(File.open(@default_config_file))
    match_config = @config["matcher"]
    Dir.chdir(File.dirname(@default_config_file)) do
      match_config['path_of'].each_pair do |k,v|
        match_config['path_of'][k] = File.expand_path(v)
      end
    end
    match_path = match_config['path_of']
    @matcher_script = File.join(File.dirname(@top_dir), "start_b5m_matcher.rb")
    @real_scd_dir = File.join(@top_dir, "real_scd")

    @logger = Logger.new(STDERR)
    @mod_count = 20000
    @validate_only = false
    @reindex_mode = 3
    @b5mo_limit = 0
    @b5mp_limit = 0
  end

  it "should always be right" do
    File.exists?(@real_scd_dir).should == true
    File.directory?(@real_scd_dir).should == true
    input_dir_list = []
    Dir.foreach(@real_scd_dir) do |d|
      next if d=='.' or d=='..'
      next unless File.directory?(File.join(@real_scd_dir,d))
      input_dir_list << d
    end
    input_dir_list.size.should > 1
    input_dir_list.sort!
    input_dir_list.each_with_index do |d,i|
      input_dir_list[i] = File.join(@real_scd_dir, d)
    end
    #max_test_num = 50000
    iter = 0
    test_pid_map = {}
    test_oid_map = {}
    test_cid_map = {}
    mock_b5m = MockB5M.new
    mock_b5m.dmp.name = "mock_b5m.dmp"
    mock_dmp = MockDm.new
    mock_dmp.name = "mock_dmp"
    mock_dmc = MockDm.new
    mock_dmc.name = "mock_dmc"
    input_dir_list.each do |input_scd_dir|
      iter+=1
      offer_scd_dir = File.join(input_scd_dir, "offer")
      comment_scd_dir = File.join(input_scd_dir, "comment")
      gen_yml_config({'scd' => "#{offer_scd_dir}", "comment_scd" => "#{comment_scd_dir}"})
      nob5mc = File.exists?(comment_scd_dir)?false:true
      unless @validate_only
        cmd = "ruby #{@matcher_script} --config #{config_file} --nologserver"
        cmd += " --nob5mc" if nob5mc
        if iter==1
          cmd = "#{cmd} --mode #{@reindex_mode}"
        end
        system(cmd)
        $?.success?.should == true
      end
      mdb_instance = nil
      mdb_instance_list = []
      Dir.foreach(mdb_dir) do |m|
        next unless m =~ /\d{14}/
        next unless File.directory?(File.join(mdb_dir,m))
        mdb_instance_list << m
      end
      mdb_instance_list.sort!
      if @validate_only
        name = mdb_instance_list[iter-1]
        break if name.nil?
        mdb_instance = File.join(mdb_dir, name)
      else
        mdb_instance_list.size.should == iter
        mdb_instance = File.join(mdb_dir, mdb_instance_list.last)
      end
      b5mo_scd = File.join(mdb_instance, "b5mo")
      b5mo_mirror_scd = File.join(mdb_instance, "b5mo_mirror")
      b5mp_scd = File.join(mdb_instance, "b5mp")
      b5mc_scd = File.join(mdb_instance, "b5mc")
      #add more pid into test_pid_map from I scd in b5mp_scd
      scd_list = ScdParser.get_scd_list(b5mp_scd)
      scd_list.size.should > 0

      #start add to mock_b5m
      mock_b5m.clear
      scd_list = ScdParser.get_scd_list(b5mo_mirror_scd)
      scd_list.size.should > 0
      puts "find #{scd_list.size} scd in b5mo_mirror"
      add_count = 0
      scd_list.each do |scd|
        puts "indexing #{scd}"
        parser = ScdParser.new(scd)
        scd_type = ScdParser.scd_type(scd)
        parser.each_with_index do |doc, i|
          if i % @mod_count==0
            puts "processing b5mo_mirror #{i} - #{add_count}"
          end
          break if @b5mo_limit>0 and i==@b5mo_limit
          sdoc = {}
          doc.each_pair do |k,v|
            sym = k.to_sym
            if sym==:Price
              sdoc[sym] = ProductPrice.parse(v)
            elsif sym==:Source
              sdoc[sym] = ProductSource.new(v)
            else
              sdoc[sym] = v
            end
          end
          docid = sdoc[:DOCID]
          pid = sdoc[:uuid]
          #valid = false
          #if test_oid_map.has_key?(docid)
            #valid = true
          #else
            #if !pid.nil? and test_pid_map.has_key?(pid)
              #valid = true
              #test_oid_map[docid]=true
            #end
          #end
          #next unless valid
          next unless is_test_pid(pid)
          add_count+=1
          if scd_type==ScdParser::INSERT_SCD
            mock_b5m.insert(sdoc)
          elsif scd_type==ScdParser::UPDATE_SCD
            mock_b5m.update(sdoc)
          elsif scd_type==ScdParser::DELETE_SCD
            mock_b5m.delete(sdoc)
          end
        end
      end
      puts "b5mo_mirror add #{add_count}"
      #add to mock_b5m end

      #start add to mock_dmp
      scd_list = ScdParser.get_scd_list(b5mp_scd)
      scd_list.size.should > 0
      puts "find #{scd_list.size} scd in b5mp"
      add_count = 0
      scd_list.each do |scd|
        puts "indexing #{scd}"
        parser = ScdParser.new(scd)
        scd_type = ScdParser.scd_type(scd)
        parser.each_with_index do |doc, i|
          if i % @mod_count==0
            puts "processing b5mp #{i} - #{add_count}"
          end
          break if @b5mp_limit>0 and i==@b5mp_limit
          sdoc = {}
          doc.each_pair do |k,v|
            sym = k.to_sym
            if sym==:Price
              sdoc[sym] = ProductPrice.parse(v)
            elsif sym==:Source
              sdoc[sym] = ProductSource.new(v)
            else
              sdoc[sym] = v
            end
          end
          docid = sdoc[:DOCID]
          next unless is_test_pid(docid)
          add_count+=1
          if scd_type==ScdParser::INSERT_SCD
            mock_dmp.insert(sdoc)
          elsif scd_type==ScdParser::UPDATE_SCD
            mock_dmp.update(sdoc)
          elsif scd_type==ScdParser::DELETE_SCD
            mock_dmp.delete(sdoc)
          end
        end
      end
      puts "b5mp add #{add_count}"
      puts "count #{mock_b5m.dmp.count}  #{mock_dmp.count}"
      mock_b5m.dmp.should dm_equal(mock_dmp)
      #add to dmp end


      #start to add to dmc
      #unless nob5mc
        #scd_list = ScdParser.get_scd_list(b5mc_output_dir)
        #scd_list.size.should > 0
        #puts "find #{scd_list.size} scd in b5mc"
        #scd_list.each do |scd|
          #puts "indexing #{scd}"
          #parser = ScdParser.new(scd)
          #scd_type = ScdParser.scd_type(scd)
          #parser.each do |doc|
            #sdoc = {}
            #doc.each_pair do |k,v|
              #sym = k.to_sym
              #sdoc[sym] = v
            #end
            #cid = sdoc[:DOCID]
            #oid = sdoc[:ProdDocid]
            #pid = sdoc[:uuid]
            #valid = false
            #if test_cid_map.has_key?(cid)
              #valid = true
            #else
              #if !pid.nil? and test_pid_map.has_key?(pid)
                #valid = true
                #test_cid_map[cid]=true
              #end
            #end
            #next unless valid
            #puts "index b5mc doc : #{sdoc}"
            #if scd_type==ScdParser::INSERT_SCD
              #mock_dmc.insert(sdoc)
            #elsif scd_type==ScdParser::UPDATE_SCD
              #mock_dmc.update(sdoc)
            #elsif scd_type==ScdParser::DELETE_SCD
              #mock_dmc.delete(sdoc)
            #end
          #end
        #end
        #mock_dmc.should dmc_match(mock_b5m.dmo)
      #end


    end

  end
end

