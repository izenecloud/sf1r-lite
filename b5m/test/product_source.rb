

class ProductSource

  attr_reader :map

  def initialize(source_str = nil)
    @map = Hash.new(0)
    unless source_str.nil?
      source_str.split(',').each do |s|
        @map[s] += 1
      end
    end
  end


  def add(source)
    source.map.each_pair do |k,v|
      @map[k]+=v
    end
  end

  def remove(source)
    source.map.each_pair do |k,v|
      @map[k]-=v
    end
  end

  #def ==(psource)
    #return to_s()==psource.to_s()
  #end

  def to_s
    result = ""
    @map.keys.sort.each do |k|
      v = @map[k]
      if v>0
        result+="," unless result.empty?
        result+=k
      end
    end

    result
  end

  def inspect
    result = ""
    @map.keys.sort.each do |k|
      v = @map[k]
      result+="," unless result.empty?
      result+="#{k}:#{v}"
    end

    result
  end

  def clone
    ps = ProductSource.new
    ps.map = @map.clone

    ps
  end


end
