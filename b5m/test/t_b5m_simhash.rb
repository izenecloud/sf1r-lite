require 'sf1-util/scd_writer'
require 'sf1-util/scd_parser'
require 'fileutils'
require 'yaml'
require 'logger'
require_relative 'mock_grouptable'
require_relative 'mock_simhash'
require_relative '../b5m_helper'
require_relative 'matcher/gt_matcher'

describe "Test B5M Simhash" do

  def prepare_empty_dir(path)
    FileUtils.rm_rf(path) if File.exist?(path)
    FileUtils.mkdir_p(path) unless File.exist?(path)
  end

  def int_to_md5(i)

    "%032d" % i
  end

  def gen_data()
    docs = []
    oarray = (1..@o_max).to_a
    oarray.shuffle!
    #parray = (1..@p_max).to_a
    #parray.shuffle!
    @index_count.times do |i|
      doc = {}
      docid = oarray[i % @o_max]
      pid = Random.rand(@p_max)+1
      doc[:DOCID] = int_to_md5(docid)
      doc[:Title] = pid
      doc[:Category] = "TC"
      doc[:Source] = @source
      @source+=1
      doc[:Price] = @price

      docs << doc
    end

    docs
  end

  before do
    @top_dir = File.dirname(__FILE__)
    @knowledge_dir = File.join(@top_dir, "test_simhash_knowledge")
    @matcher = B5MPath.matcher
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
    @cma = match_path['cma']
    @simhash_dic = match_path['simhash_dic']
    @mock_gt = MockGrouptable.new
    @mock_simhash = MockSimhash.new
    @o_max = 40000
    @p_max = 1000
    @index_count = 100
    @iteration = 200
    @price = 100.0
    @source = 1
    @logger = Logger.new(STDOUT)
  end

  it "should always be correct" do
    #currently sf1r simhash ignore the later duplicate docs

    #generate pid dic first
    File.open(@simhash_dic, 'w') do |f|
      (1..@o_max).each do |pid|
        f.puts int_to_md5(pid)
      end
    end
    category_dir = @knowledge_dir
    reindex_prob = 0.05
    reindex = true

    iter = 0
    occurrence = {}
    while iter<@iteration
      sleep(1.0)
      iter+=1
      puts "iterator #{iter}"
      if reindex
        puts "reindex!"
        prepare_empty_dir(@knowledge_dir)
        @mock_gt.clear
        @mock_simhash.clear
      end
      type_file = File.join(category_dir, "type")
      ofs = File.open(type_file, 'w')
      ofs.puts("similarity")
      ofs.close
      regex_file = File.join(category_dir, "category")
      ofs = File.open(regex_file, 'w')
      ofs.close
      a_scd = File.join(@knowledge_dir, "A")
      time_str = Time.now.strftime("%Y%m%d%H%M%S")
      a_scd_instance = File.join(a_scd, time_str)
      matchdone_file = File.join(category_dir, "match.done")
      match_file = File.join(category_dir, "match")
      prepare_empty_dir(a_scd_instance)
      FileUtils.rm_rf(matchdone_file)
      FileUtils.rm_rf(match_file)
      docs = gen_data()
      writer = ScdWriter.new(a_scd_instance, "I")
      docs.each do |doc|
        docid = doc[:DOCID]
        next if occurrence.has_key?(docid)
        occurrence[docid] = 0
        writer.append(doc)
        @mock_simhash.add_doc(doc)
        @mock_gt.add(doc[:DOCID])
      end
      @mock_simhash.flush
      writer.close
      cmd = "#{@matcher} -I -C #{@cma} -S #{a_scd_instance} -K #{category_dir} --dictionary #{@simhash_dic}"
      #puts cmd
      #if iter>1
        #false.should == true
      #end
      system(cmd)
      $?.success?.should == true
      File.exists?(matchdone_file).should == true
      File.exists?(match_file).should == true
      IO.readlines(match_file).each do |line|
        v = line.split(',')
        id = v[0]
        gid = v[1]
        @mock_gt.add(id, gid)
      end

      @mock_gt.should gt_equal(@mock_simhash.gt)
      match_file_instance = File.join(category_dir, "match_#{time_str}")
      FileUtils.mv(match_file, match_file_instance)

      #reset reindex
      ran = Random.rand()
      if ran<reindex_prob
        reindex = true
      else
        reindex = false
      end
    end
  end

end

