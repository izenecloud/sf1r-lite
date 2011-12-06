# Author:: Jun
# Date:: 2011-12-05

# scan scd files
class SCDScanner
  def initialize(dir, propDocId, propCategory)
    @dir = dir
    @propDocId, @propCategory = propDocId, propCategory
  end

  def scan
    fileCount = 0
    docCount = 0
  
    Dir.glob("#{@dir}/*.SCD") do |inputName|
      File.open(inputName, "r") do |inputFile|
        fileCount += 1
    
        while line = inputFile.gets
          line = line.chomp!

          case line
          when /^<#{@propDocId}>(.+)$/o
            docID = $~[1]
            docCount += 1

            if docCount % 1000 == 0
              break
              print "\rscanning #{fileCount} SCD, #{docCount} docs"
            end

          when /^<#{@propCategory}>(.+)$/o
            catList = $~[1]
            yield docID, catList
          end
        end
    
      end
    end
  
    puts "\rscanning #{fileCount} SCD, #{docCount} docs"
  end

end
