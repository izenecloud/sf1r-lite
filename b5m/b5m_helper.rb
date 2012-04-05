
class CategoryTask
  ATT = 1 #attribute indexing approach
  COM = 2 #complete match approach
  SIM = 3 #similarity based approach

  attr_reader :type, :category, :cid, :info, :valid, :regex

  def initialize(line)
    str = line
    @type = ATT
    @category = ""
    @cid = ""
    @info = {}
    @valid = true
    @regex = true
    if str.start_with?("#")
      str = str[1..-1]
      @valid = false
    end
    a = str.split(',')
    @category = a[0]
    @cid = a[1]
    if a.size>=4 and a[2]=="COMPLETE"
      @type = COM
      @info['name'] = a[3]
    elsif a.size>=3 and a[2]=="SIMILARITY"
      @type = SIM
    elsif a.size>=3 and a[2]=="DISABLE"
      @info['disable'] = true
    end

    if @category=="OPPOSITE" or @category=="OPPOSITEALL"
      @regex = false
    end
  end

  def to_s
    r = "#{@category},#{@cid}"
    if @type==ATT
      unless @info['disable'].nil?
        r += ",DISABLE"
      end
    elsif @type==COM
      r += ",COMPLETE,#{@info['name']}"
    else
      r += ",SIMILARITY"
    end

    r
  end

end

class B5MPath
  def self.scd_merger

    File.join( File.dirname(File.dirname(__FILE__)), "scripts", "ScdMerger")
  end

  def self.matcher

    File.join( File.dirname(__FILE__), "b5m_matcher")
  end
end

