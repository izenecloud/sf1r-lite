require 'rubygems'
require 'rexml/document'
require 'iconv'


class Category
  attr_accessor :name, :labels
  
  def initialize
    @name = ""
    @labels = []
  end
  
  def set_label(label)
    @labels = label.split(',')
#     puts labels
    regex_str = ""
    @labels.each do |l|
      if regex_str.length>0
        regex_str += "|"
      end
      regex_str += l
    end
#     puts regex_str
    @regex = Regexp.new("(#{regex_str})")
  end
  
  def count(str)
    str.scan(@regex).length
  end
  
  def score(str)
    c = count(str)
    c.to_f/Math.log(@labels.length.to_f)
  end
  
end

class Classifier
  def initialize
    @categories = []
  end
  
  def load(owl_file)
    xml = File.read(owl_file)
    doc = REXML::Document.new(xml)
    doc.elements.each('rdf:RDF/owl:Class') do |c|
      name = c.attribute("rdf:ID").to_s
      label_str = ""
      c.elements.each("rdfs:label") do |l|
        label_str += l.get_text.to_s
      end
      if label_str.length>0
        category = Category.new
        category.name = name
        category.set_label(label_str)
        @categories << category
#         puts category.score("食品专卖")
      end
    end
  end
  
  def get_class(str)
    name = ""
    max_score = 0.0
    @categories.each do |c|
      score = c.score(str)
      if score>max_score
        name = c.name
        max_score = score
      end
    end
    name
  end
  
  
end

# classifer = Classifier.new
# classifer.load "/home/jarvis/codebase/sf1r-engine/tuan.owl"
# puts classifer.get_class("中国食品专卖, 火锅, 日本料理")
