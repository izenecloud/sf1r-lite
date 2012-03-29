require 'sf1-util/scd_writer'
require 'sf1-util/scd_parser'
require 'fileutils'
require_relative '../b5m_helper'

describe "Test Scd Merger" do

  top_dir = File.dirname(__FILE__)

  it "should merge to one scd" do
    scd_merger = B5MPath.scd_merger
    input_scd = File.join(top_dir, "input_scd")
    FileUtils.rm_rf(input_scd) if File.exist?(input_scd)
    Dir.mkdir(input_scd)
    output_scd = File.join(top_dir, "output_scd")
    FileUtils.rm_rf(output_scd) if File.exist?(output_scd)
    Dir.mkdir(output_scd)
    writer = ScdWriter.new(input_scd, "D")
    doc = {}
    (1..100).each do |i|
      doc.clear
      doc['DOCID'] = i.to_s
      writer.append(doc)
    end
    writer.close
    writer = ScdWriter.new(input_scd, "U")
    doc = {}
    (91..100).each do |i|
      doc.clear
      doc['DOCID'] = i.to_s
      doc['Title'] = "u1-#{i.to_s}"
      writer.append(doc)
    end
    writer.close

    writer = ScdWriter.new(input_scd, "I")
    doc = {}
    (1..100).each do |i|
      doc.clear
      doc['DOCID'] = i.to_s
      doc['Title'] = "i1-#{i.to_s}"
      writer.append(doc)
    end
    writer.close

    writer = ScdWriter.new(input_scd, "U")
    doc = {}
    (91..100).each do |i|
      doc.clear
      doc['DOCID'] = i.to_s
      doc['Title'] = "u2-#{i.to_s}"
      writer.append(doc)
    end
    writer.close

    writer = ScdWriter.new(input_scd, "D")
    doc = {}
    (86..95).each do |i|
      doc.clear
      doc['DOCID'] = i.to_s
      writer.append(doc)
    end
    writer.close
    
    system("#{scd_merger} #{input_scd} DOCID,Title #{output_scd}")

    scd_list = []
    Dir.foreach(output_scd) do |scd|
      next unless scd.end_with? ".SCD"
      scd_list << File.join(output_scd, scd)
    end

    scd_list.size.should == 1
    parser = ScdParser.new(scd_list.first)
    doc_list = []
    parser.each do |d|
      doc_list << d
    end

    #doc_list.each do |d|
      #puts d
    #end

    doc_list.sort! {|x, y| x['DOCID'].to_i <=> y['DOCID'].to_i}

    doc_list.each do |doc|
      id = doc['DOCID'].to_i
      c = (id<=85 or id>=96)
      c.should be true
      if id<=85
        doc['Title'].should == "i1-#{id}"
      else
        doc['Title'].should == "u2-#{id}"
      end
      #puts doc
    end


    #do clean
    FileUtils.rm_rf(input_scd) if File.exist?(input_scd)
    FileUtils.rm_rf(output_scd) if File.exist?(output_scd)
  end


  it "should merge to multiple scds" do
    scd_merger = B5MPath.scd_merger
    input_scd = File.join(top_dir, "input_scd")
    FileUtils.rm_rf(input_scd) if File.exist?(input_scd)
    Dir.mkdir(input_scd)
    output_scd = File.join(top_dir, "output_scd")
    FileUtils.rm_rf(output_scd) if File.exist?(output_scd)
    Dir.mkdir(output_scd)
    writer = ScdWriter.new(input_scd, "D")
    doc = {}
    (1..100).each do |i|
      doc.clear
      doc['DOCID'] = i.to_s
      writer.append(doc)
    end
    writer.close
    writer = ScdWriter.new(input_scd, "U")
    doc = {}
    (91..100).each do |i|
      doc.clear
      doc['DOCID'] = i.to_s
      doc['Title'] = "u1-#{i.to_s}"
      writer.append(doc)
    end
    writer.close

    writer = ScdWriter.new(input_scd, "I")
    doc = {}
    (51..95).each do |i|
      doc.clear
      doc['DOCID'] = i.to_s
      doc['Title'] = "i1-#{i.to_s}"
      writer.append(doc)
    end
    writer.close

    writer = ScdWriter.new(input_scd, "U")
    doc = {}
    (88..92).each do |i|
      doc.clear
      doc['DOCID'] = i.to_s
      doc['Title'] = "u2-#{i.to_s}"
      writer.append(doc)
    end
    writer.close

    writer = ScdWriter.new(input_scd, "D")
    doc = {}
    (86..91).each do |i|
      doc.clear
      doc['DOCID'] = i.to_s
      writer.append(doc)
    end
    writer.close
    
    system("#{scd_merger} #{input_scd} DOCID,Title #{output_scd} --gen-all")

    scd_list = []
    Dir.foreach(output_scd) do |scd|
      next unless scd.end_with? ".SCD"
      scd_list << File.join(output_scd, scd)
    end

    scd_list.size.should == 3
    insert_scd = nil
    update_scd = nil
    delete_scd = nil
    scd_list.each do |scd|
      if ScdParser.scd_type(scd) == ScdParser::INSERT_SCD
        insert_scd = scd
      elsif ScdParser.scd_type(scd) == ScdParser::UPDATE_SCD
        update_scd = scd
      elsif ScdParser.scd_type(scd) == ScdParser::DELETE_SCD
        delete_scd = scd
      end
    end
    insert_scd.should_not be nil
    update_scd.should_not be nil
    delete_scd.should_not be nil

    insert_doc_list = []
    update_doc_list = []
    delete_doc_list = []
    parser = ScdParser.new(insert_scd)
    parser.each do |doc|
      insert_doc_list << doc
    end
    parser = ScdParser.new(update_scd)
    parser.each do |doc|
      update_doc_list << doc
    end
    parser = ScdParser.new(delete_scd)
    parser.each do |doc|
      delete_doc_list << doc
    end

    delete_doc_list.size.should == 56
    update_doc_list.size.should == 6
    insert_doc_list.size.should == 38


    #do clean
    FileUtils.rm_rf(input_scd) if File.exist?(input_scd)
    FileUtils.rm_rf(output_scd) if File.exist?(output_scd)
  end


end
