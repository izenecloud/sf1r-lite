# Author:: Jun
# Date:: 2011-12-05

# count docs for each category
class CategoryCounter
  attr_reader :countMap # map from category to count
  attr_reader :totalCount

  # "maxLevel" starts from 0
  def initialize(maxLevel, delim)
    @maxLevel, @delim = maxLevel, delim
    @countMap = Hash.new
    @totalCount = 0
  end

  def add(catList)
    catList.strip!
    return if catList.empty?
  
    category = extractCategory(catList)
    @totalCount += 1

    if @countMap[category]
      @countMap[category] += 1
    else
      @countMap[category] = 1
    end
  end

  def extractCategory(catList)
    result = ""

    catList.split(@delim).each_with_index do |category, level|
      result << category << "_"
      break if level == @maxLevel
    end
  
    result
  end

end
