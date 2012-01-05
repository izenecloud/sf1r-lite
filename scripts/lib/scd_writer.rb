
class ScdWriter
  
  def initialize(dir, type = 'I')
    time = Time::now
    filename = time.strftime("B-00-%Y%m%d%H%M-%S000-#{type}-C.SCD")
    f = dir+"/"+filename
    @o_file = File.new(f, "w")
  end
  
  def append(doc)
    @o_file.puts "<DOCID>#{doc['DOCID']}"
    doc.each do |key, value|
      if key=="DOCID"
        next
      end
      @o_file.puts "<#{key}>#{value}"
    end
#     @o_file.flush
  end
  
  def close
    @o_file.close
  end
  
end
  
