

class ScdParser
  
  def initialize(file)
    @file = file
    @max_property_name_len = 15
    @property_name_regex = Regexp.new("^[A-Za-z]{1,#{@max_property_name_len}}$")
    
  end
  
  def each
    doc = {}
    last_property_name = ""
    i_file = File.open(@file, "r")
    i_file.each_line do |line|
      line.strip!
      property_name = ""
      property_value = line
      if line.start_with? '<'
        right = line.index('>')
        if right!=nil
          right -= 1
          property_name = line[1..right]
          right += 2
          property_value = line[right..-1]
          if !property_name.match(@property_name_regex)
            property_name = ""
            property_value = line
          end
        end
#         if property_name.length==0
#           puts "WARNING line : #{line}"
#         end
      end
      if property_name == ""
        doc[last_property_name] += "\n"
        doc[last_property_name] += property_value
      else
        if property_name == 'DOCID'
          if doc.size>0 
            yield doc
            doc.clear
          end
        end
        doc[property_name] = property_value
        last_property_name = property_name
      end
    end #each line end
    if doc.size>0
      yield doc
    end
    i_file.close
  end
end

