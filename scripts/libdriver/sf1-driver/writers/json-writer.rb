#--
# Author::  Ian Yang
# Created:: <2010-06-12 17:14:39>
#++
#
# write data into JSON

require 'json'

class Sf1Driver
  module JsonWriter
    def writer_serialize(data)
      return JSON.dump(data)
    end
  end
end
