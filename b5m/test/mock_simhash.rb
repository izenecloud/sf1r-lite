
require_relative 'mock_grouptable'

class MockSimhash
  #currently b5m simhash ignore the later duplicate docs, as sf1r does.
  attr_reader :gt,:docs
  def initialize
    @gt = MockGrouptable.new
    @docs = []
  end

  def add_doc(doc)
    return if doc[:DOCID].nil?
    return if doc[:Title].nil?
    return if @gt.map.has_key?(doc[:DOCID])
    @docs << doc
  end

  def flush
    #sort and split by Title
    map = {}
    @docs.each do |doc|
      #puts "flush doc : #{doc[:DOCID]}  #{doc[:Title]}"
      title = doc[:Title]
      map[title] ||= []
      map[title] << doc[:DOCID]
    end
    @docs.clear
    map.each_pair do |k,v|
      v.each do |id|
        @gt.add(id, k)
      end
    end
  end

  def clear
    @gt.clear
    @docs.clear
  end
end

