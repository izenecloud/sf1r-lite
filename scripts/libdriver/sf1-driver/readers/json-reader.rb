#--
# Author::  Ian Yang
# Created:: <2010-06-12 17:12:56>
#++
#
# read data from JSON

require 'rubygems'
require 'json'

class Sf1Driver
  module JsonReader
    def reader_deserialize(data)
      return JSON.parse(data)
    end
  end
end
