# encoding: UTF-8
#require 'uuid'
require 'sf1-util/mock_dm'
require_relative 'product_price'
require_relative 'product_source'

class MockB5M < MockDm

  attr_accessor :dmo, :dmp


  def initialize
    #@dmo = MockDm.new([:uuid])
    @dmo = MockDm.new
    @dmp = MockDm.new
    #@uuid_gen = UUID.new
    #@uuid_map = {}
  end

  #def self.get_product_doc(doc)
    #sdoc = {}
    #doc.each_pair do |k,v|
      #sym = k
      #unless sym.is_a?Symbol
        #sym = sym.to_sym
      #end
      #sym = k.to_sym
      #if sym==:Price
        #sdoc[sym] = ProductPrice.new(v.to_i)
      #elsif sym==:Source
        #sdoc[sym] = ProductSource.new(v)
      #else
        #sdoc[sym] = v
      #end
    #end
  #end

  def index(path)
    scd_list = ScdParser.get_scd_list(path)
    return if scd_list.empty?
    puts "find #{scd_list.size} scd"
    scd_list.each do |scd|
      puts "indexing #{scd}"
      parser = ScdParser.new(scd)
      scd_type = ScdParser.scd_type(scd)
      parser.each do |doc|
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
        if scd_type==ScdParser::INSERT_SCD
          insert(sdoc)
        elsif scd_type==ScdParser::UPDATE_SCD
          update(sdoc)
        elsif scd_type==ScdParser::DELETE_SCD
          delete(sdoc)
        end
      end
    end

  end

  def insert(doc)
    oid = doc[:DOCID]
    pid = doc[:uuid]
    return false if oid.nil?
    return false if pid.nil?
    return false unless @dmo.insert(doc)
    pdoc = @dmp.get(pid)
    unless pdoc.nil?
      p "mockb5m before insert dmp #{pdoc}"
    end
    pdoc = make_pdoc(pdoc, [doc], [])
    p "mockb5m insert or update dmp #{pdoc}"
    @dmp.update(pdoc)
  end

  def p(str)
    return
    Object.p(str)
  end

  #def create_doc(doc, same_group_docid = nil)
    #preprocess_doc(doc)
    #if !same_group_docid.nil?
      #same_doc = @dmm.get(same_group_docid)
      #uuid = same_doc[:uuid]
    #else
      #uuid = @uuid_gen.generate
    #end
    #doc[:uuid] = uuid
    #@dmm.insert(doc)
    #if !same_group_docid.nil?
      #doca = @dma.get(uuid)
      #pricea = doca[:Price].clone
      #price = doc[:Price]
      #pricea.add(price)
      #update_doc = {:DOCID => uuid, :Price => pricea, :itemcount => (doca[:itemcount]+1)}
      #@dma.update(update_doc)
    #else
      #doca = doc.clone
      #doca.delete(:uuid)
      #doca[:DOCID] = uuid
      #doca[:itemcount] = 1
      #@dma.insert(doca)
    #end
  #end

  def update(doc)
    #delete(doc)
    #insert(doc)
    
    oid = doc[:DOCID]
    return false if oid.nil?
    old_odoc = @dmo.get(oid)
    if old_odoc.nil?
      return insert(doc)
    else
      old_pid = old_odoc[:uuid]
      pid = doc[:uuid]
      pid = old_pid if pid.nil?
      doc[:uuid] = pid
      new_odoc = old_odoc.clone
      new_odoc.merge!(doc)
      @dmo.update(new_odoc)
      old_pdoc = @dmp.get(old_pid)
      p "mockb5m before update #{old_pdoc}"
      old_pdoc = make_pdoc(old_pdoc, [], [old_odoc])
      pdoc = @dmp.get(pid)
      pdoc = make_pdoc(pdoc, [new_odoc], [])
      if pid==old_pid
        old_pdoc = make_pdoc(old_pdoc, [new_odoc], [])
        pdoc = make_pdoc(pdoc, [], [old_odoc])
      end
      if old_pdoc[:itemcount]>0
        p "mockb5m update dmp #{pdoc}"
        @dmp.update(old_pdoc)
      else
        p "mockb5m delete dmp #{pdoc}"
        @dmp.delete(old_pdoc)
      end
      p "mockb5m update dmp #{pdoc}"
      @dmp.update(pdoc)
    end

    true
  end

  def delete(doc)
    oid = doc[:DOCID]
    return false if oid.nil?
    old_odoc = @dmo.get(oid)
    return false if old_odoc.nil?
    pid = old_odoc[:uuid]
    @dmo.delete(doc)
    pdoc = @dmp.get(pid)
    p "mockb5m before delete for update #{pdoc}"
    pdoc = make_pdoc(pdoc, [], [old_odoc])
    if pdoc[:itemcount]>0
      p "mockb5m update dmp #{pdoc}"
      @dmp.update(pdoc)
    else
      p "mockb5m delete dmp #{pdoc}"
      @dmp.delete(pdoc)
    end

    true
  end


  def clear
    @dmo.clear
    @dmp.clear
  end


  private
  def make_pdoc(pdoc, append_list, remove_list)
    new_pdoc = nil
    unless pdoc.nil?
      new_pdoc = pdoc.clone
      pprice = new_pdoc[:Price] or ProductPrice.new
      psource = new_pdoc[:Source] or ProductSource.new
      pitemcount = new_pdoc[:itemcount] or 0
      ptitle = new_pdoc[:Title]
      pid = new_pdoc[:DOCID]
    else
      pprice = ProductPrice.new
      psource = ProductSource.new
      pitemcount = 0
      ptitle = nil
      pid = nil
    end
    append_list.each do |odoc|
      pid = odoc[:uuid] if pid.nil?
      ptitle = odoc[:Title] if ptitle.nil?
      pprice.add(odoc[:Price]) unless odoc[:Price].nil?
      psource.add(odoc[:Source]) unless odoc[:Source].nil?
      pitemcount+=1
    end
    remove_list.each do |odoc|
      pprice.remove(odoc[:Price]) unless odoc[:Price].nil?
      psource.remove(odoc[:Source]) unless odoc[:Source].nil?
      pitemcount-=1
    end
    new_pdoc = {} if new_pdoc.nil?
    new_pdoc[:Price] = pprice
    new_pdoc[:Source] = psource
    new_pdoc[:itemcount] = pitemcount
    new_pdoc[:Title] = ptitle
    new_pdoc[:DOCID] = pid

    new_pdoc
  end

  #def preprocess_doc(doc)
    #if doc[:Price].is_a? String
      #doc[:Price] = ProductPrice.parse(doc[:Price])
    #elsif doc[:Price].is_a? Float
      #doc[:Price] = ProductPrice.new(doc[:Price])
    #end
  #end

  #def get_uuid_count(uuid)
    #doca = @dma.get(uuid)
    #return 0 if doca.nil?

    #doca[:itemcount]
  #end

  #def get_uuid_doclist(uuid, remove_docid)
    #doc_list = @dmm.search(:uuid, uuid)
    #unless remove_docid.nil?
      #doc_list.delete_if do |doc|
        #doc[:DOCID] == remove_docid
      #end
    #end

    #doc_list
  #end

  #def append(doc_list, resource)
    #uuid = resource[:DOCID]
    ##puts "append uuid: #{uuid}"
    #new_doc = resource.clone
    #unless doc_list.empty?
      #first_doc = doc_list.first
      #first_doc.each_pair do |k,v|
        #new_doc[k] = v unless new_doc.has_key?(k)
      #end
    #end
    #new_doc.delete(:uuid)
    #new_doc[:Price] = ProductPrice.new
    #new_doc[:itemcount] = 0
    #new_doc[:manmade] = 1
    #type = 1 #insert
    #uuid_count = get_uuid_count(uuid)
    #type = 2 unless uuid_count==0

    #doc_list.each do |doc|
      #docid = doc[:_id]
      #doc_uuid = doc[:uuid]
      #doc_uuid_count = get_uuid_count(doc_uuid)
      #update_docm = {:DOCID => doc[:DOCID], :uuid => uuid}
      #@dmm.update(update_docm)
      ## doc_uuid_count must > 0
      #if doc_uuid_count==1
        #del_doca = {:DOCID => doc_uuid}
        #@dma.delete(del_doca)
      #else
        #uuid_doclist = get_uuid_doclist(doc_uuid, doc[:DOCID])
        #update_price = ProductPrice.new
        #uuid_doclist.each do |uuid_doc|
          #update_price.add(uuid_doc[:Price])
        #end
        #update_doc = {:DOCID => doc_uuid, :Price => update_price, :itemcount => uuid_doclist.size}
        #@dma.update(update_doc)
      #end
      #new_doc[:Price].add(doc[:Price])
      #new_doc[:itemcount] += 1
    #end
    #if type==1
      #@dma.insert(new_doc)
    #else
      #@dma.update(new_doc)
    #end
  #end

end

