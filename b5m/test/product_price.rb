

class ProductPrice
  attr_accessor :min, :max
  def initialize(min=-1.0, max=min)
    @min = min
    @max = max
  end

  def self.parse(str)
    
    if str.index('-').nil?
      min = str.to_f
      max = min
    else
      s = str.split('-')
      min = s[0].to_f
      max = s[1].to_f
    end

    ProductPrice.new(min, max)
  end

  def valid
    @min>=0.0 and @max>=0.0 and @max<=1000000.0
  end

  def add(price)
    if !valid
      if price.valid
        @min = price.min
        @max = price.max
      end
    elsif price.valid
      @min = [@min, price.min].min
      @max = [@max, price.max].max
    end
  end

  def remove(price)
    #do nothing now
  end

  def ==(price)
    if price.is_a? String
      price = ProductPrice.parse(price)
    end
    return true if !valid and !price.valid
    return false if !valid or !price.valid
    return true if @min==price.min and @max==price.max

    false
  end

  def to_s
    return "" unless valid
    return @min.to_s if @min==@max

    "#{@min}-#{@max}"
  end

  def clone
    return ProductPrice.new(@min, @max)
  end


end
