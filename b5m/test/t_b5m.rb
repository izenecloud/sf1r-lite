require 'fileutils'
require 'sf1-util/scd_writer'
require 'sf1-util/scd_parser'
require 'sf1-util/mock_dm'
require 'yaml'
require 'logger'
require_relative '../b5m_helper'
require_relative 'mock_b5m'
require_relative 'matcher/dm_matcher'

describe "B5M Tester" do


  before do
    @top_dir = File.dirname(__FILE__)
    @config_file = File.join(@top_dir, "config.yml")
    @config = YAML::load(File.open(@config_file))
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
    @db_path = File.join(@work_dir, 'db')
    @odb = File.join(@db_path, 'odb')
    @pdb = File.join(@db_path, 'pdb')
    @mdb = File.join(@db_path, 'mdb')
    @tmp_dir = match_path['tmp']
    @result_file = match_path['result']
    @train_scd = match_path['train_scd']
    @scd = match_path['scd']
    @comment_scd = match_path['comment_scd']
    @cma = match_path['cma']
    @b5mo_scd = File.join(match_path['b5mo'])
    @b5mp_scd = File.join(match_path['b5mp'])
    @b5mc_scd = File.join(match_path['b5mc'])
    [@b5mo_scd, @b5mp_scd, @b5mc_scd].each do |path|
      prepare_empty_dir(path)
    end
    #Dir.mkdir(@work_dir) unless File.exist?(@work_dir)
    #Dir.mkdir(@db_path) unless File.exist?(@db_path)
    #Dir.mkdir(@mdb) unless File.exist?(@mdb)
    @matcher_program = B5MPath.matcher

    @o_max = 20000
    @p_max = 500
    @price_max = 10000
    @source_list = ["SA", "SB", "SC", "SD", "SE"]
    @logger = Logger.new(STDOUT)
  end

  def prepare_empty_dir(path)
    FileUtils.rm_rf(path) if File.exist?(path)
    FileUtils.mkdir_p(path) unless File.exist?(path)
  end

  def random_price
    fprice = Random.rand(@price_max)+1

    ProductPrice.new(fprice)
  end

  def gen_docs(count, probs)
    idocs, udocs, ddocs = [],[],[]
    iprob, uprob, dprob =  probs[0], probs[0]+probs[1], probs[0]+probs[1]+probs[2]
    oarray = (1..@o_max).to_a
    oarray.shuffle!
    oindex = 0
    count.times do
      r = Random.rand
      type = ScdParser::NOT_SCD
      docid = oarray[oindex]
      oindex+=1
      if r<iprob
        type = ScdParser::INSERT_SCD
      elsif r<uprob
        type = ScdParser::UPDATE_SCD
      elsif r<dprob
        type = ScdParser::DELETE_SCD
      end
      if type==ScdParser::INSERT_SCD
        doc = {}
        doc[:DOCID] = ("%032d" % docid)
        pid = Random.rand(@p_max)+1
        doc[:uuid] = ("%032d" % pid)
        doc[:Title] = Time.now.to_s
        doc[:Price] = random_price
        source_str = @source_list[Random.rand(@source_list.size)]
        doc[:Source] = ProductSource.new(source_str)
        idocs << doc
      elsif type==ScdParser::UPDATE_SCD
        doc = {}
        doc[:DOCID] = ("%032d" % docid)
        doc[:Price] = random_price
        udocs << doc
      elsif type==ScdParser::DELETE_SCD
        doc = {}
        doc[:DOCID] = ("%032d" % docid)
        ddocs << doc
      end
    end

    return idocs, udocs, ddocs
  end

  it "should always be right" do
    #parameters
    iteration = 500
    reindex_prob = 0.05

    #begin test, given b5mo scd, to test b5mp and log server
    [@mdb, @odb, @pdb].each do |db|
      prepare_empty_dir(db)
    end
    mock_b5m = MockB5M.new
    mock_dmp = MockDm.new
    reindex = true
    iter = 0
    while iter<iteration
      sleep(1.0)
      iter+=1
      if reindex
        FileUtils.rm_rf(@work_dir) if File.exist?(@work_dir)
        mock_b5m.clear
        mock_dmp.clear
      end
      #prepare_empty_dir(@scd)
      time_str = Time.now.strftime("%Y%m%d%H%M%S")
      mdb_instance = File.join(@mdb, time_str)
      b5mo_scd = File.join(@b5mo_scd, time_str)
      b5mp_scd = File.join(@b5mp_scd, time_str)
      prepare_empty_dir(mdb_instance)
      prepare_empty_dir(b5mo_scd)
      prepare_empty_dir(b5mp_scd)

      probs = [0.2, 0.7, 0.1]
      index_count = @o_max/10
      if reindex
        probs = [1.0,0.0,0.0]
        index_count = @o_max/4
      end

      insert_docs, update_docs, delete_docs = gen_docs(index_count, probs)

      @logger.info "iteration #{iter}"
      @logger.info "insert_docs #{insert_docs.size}"
      @logger.info "update_docs #{update_docs.size}"
      @logger.info "delete_docs #{delete_docs.size}"

      unless insert_docs.empty?
        writer = ScdWriter.new(b5mo_scd, "I")
        insert_docs.each do |doc|
          writer.append(doc)
        end
        writer.close
      end
      unless update_docs.empty?
        writer = ScdWriter.new(b5mo_scd, "U")
        update_docs.each do |doc|
          writer.append(doc)
        end
        writer.close
      end
      unless delete_docs.empty?
        writer = ScdWriter.new(b5mo_scd, "D")
        delete_docs.each do |doc|
          writer.append(doc)
        end
        writer.close
      end
      system("#{@matcher_program} --uue-generate --b5mo #{b5mo_scd} --uue #{mdb_instance}/uue --odb #{@odb}")
      $?.success?.should be true
      system("#{@matcher_program} --b5mp-generate --b5mo #{b5mo_scd} --b5mp #{b5mp_scd} --uue #{mdb_instance}/uue --odb #{@odb} --pdb #{@pdb}")
      $?.success?.should be true
      insert_docs.each do |doc|
        mock_b5m.insert(doc)
      end
      update_docs.each do |doc|
        mock_b5m.update(doc)
      end
      delete_docs.each do |doc|
        mock_b5m.delete(doc)
      end
      mock_dmp.index(b5mp_scd)
      puts "mock_dmp count : #{mock_dmp.count}"

      mock_b5m.dmp.should dm_equal(mock_dmp)


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

