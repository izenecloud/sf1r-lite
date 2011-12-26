#--
# Author::  Ian Yang
# Created:: <2010-07-13 16:41:10>
#++
#
# Helper methods

require "stringio"

class Sf1Driver
  module Helper
    module_function

    def num_bytes(str) #:nodoc:
      s = StringIO.new(str)
      s.seek(0, IO::SEEK_END)
      s.tell
    end

    def header_size #:nodoc:
      2 * num_bytes([1].pack('N'))
    end

    def camel(str) #:nodoc:
      str.split("-").map{|s| s.capitalize}.join
    end
  end
end
