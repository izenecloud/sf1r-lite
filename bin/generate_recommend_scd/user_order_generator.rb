# Author:: Jun
# Date:: 2011-12-05

require 'set'

# generate users and orders
class UserOrderGenerator
  attr_reader :userCount, :orderCount
  AVG_USER_ORDER = 20

  def initialize(userFileName, orderFileName, catCounter)
    @userFile = File.open(userFileName, "w")
    @orderFile = File.open(orderFileName, "w")
    @catCounter = catCounter
    @userIDSet = Set.new
    @catMaxUser = Hash.new # map from category to max user count
    @userCount, @orderCount = 0, 0

    puts "\ntotal #{@catCounter.totalCount} docs which contain category values"
    puts "printing doc count for each category:"
    puts "category\tdoc count\tpercentage\tmax user count"
    @catCounter.countMap.each do |cat, count|
      userNum = count / AVG_USER_ORDER
      userNum += 1 if userNum == 0
      @catMaxUser[cat] = userNum
      puts "#{cat}\t#{count}\t#{count*100/@catCounter.totalCount}%\t#{userNum}"
    end
  end

  def close
    @userFile.close
    @orderFile.close
  end

  def add(docID, catList)
    catList.strip!
    return if catList.empty?

    category = @catCounter.extractCategory(catList)
    maxUserID = @catMaxUser[category]
    return if maxUserID.nil?

    #puts "category: #{category}, maxUserID: #{maxUserID}"
    randID = rand(maxUserID)
    userID = "#{category}#{randID}"
    #puts userID

    @orderFile.puts "<USERID>#{userID}"
    @orderFile.puts "<ITEMID>#{docID}"
    @orderCount += 1

    if @userIDSet.add? userID
      @userFile.puts "<USERID>#{userID}"
      @userCount += 1
    end
  end

end
