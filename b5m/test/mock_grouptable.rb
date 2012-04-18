
class MockGrouptable
  attr_reader :map
  def initialize
    @map = {}
    @gid = 1
  end

  def add(id_array, gid = nil)
    if !id_array.is_a? Array
      id_array = [id_array]
    end
    if gid.nil?
      id_array.each do |id|
        gid = @map[id]
        break unless gid.nil?
      end
    end
    if gid.nil?
      gid = @gid
      @gid+=1
    end
    id_array.each do |id|
      map[id] = gid
    end
  end

  def has_key?(id)

    @map.has_key?(id)
  end

  def clear
    @map.clear
    @gid = 1
  end
end

