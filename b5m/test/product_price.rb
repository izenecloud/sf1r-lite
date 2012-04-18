

class ProductPrice
  attr_accessor :min, :max
  def initialize(min=-1.0, max=min)
    @min = min
    @max = max
  end

  def self.parse(str)
    
    if str.index('-').nil?
      min = str_to_f(str)
      max = min
    else
      s = str.split('-')
      min = str_to_f(s[0])
      max = str_to_f(s[1])
    end

    ProductPrice.new(min, max)
  end

  def self.str_to_f(str)
    f = Float(str) rescue -1.0

    f
  end

  def valid
    @min>=0.0 and @max>=0.0
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
