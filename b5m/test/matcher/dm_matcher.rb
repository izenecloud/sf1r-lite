require "rspec/matchers"
require "builder"

module RSpec
  module Matchers
    class DMMatcher

      def initialize(dm, properties)
        @dm = dm
        @error = ""
        @property_map={}
        properties.each do |p|
          @property_map[p]=true
        end
      end

      def valid_property(p)
        return true if @property_map.empty?
        return @property_map.has_key?(p)
      end
      
      def matches?(another_dm)
        if @dm.count!=another_dm.count
          @error = "count in caller and instance #{another_dm.count} - #{@dm.count}"
          return false
        end
        if @dm.count==0
          return true
        end

        @dm.docs.each do |doc|
          next if doc.nil?
          docid = doc[:DOCID]
          another_doc = another_dm.get(docid)
          if another_doc.nil?
            @error = "can not find docid #{docid} in caller dm"
            return false
          end
          diff_key = nil
          doc.each_pair do |k,v|
            next if !valid_property(k)
            #puts "key : #{k.class}, #{k}"
            if v.to_s!=another_doc[k].to_s
              diff_key = k
              break
            end
          end
          unless diff_key.nil?
            @error = "#{docid} diff in key #{diff_key} [#{another_dm.name} - #{@dm.name}] : #{another_doc.to_s} *** #{doc.to_s}"
            return false
          end
        end

        true
      end

      def failure_message_for_should
        "#{@error}"
      end

      private

    end

    def dm_equal(dm, properties = [:DOCID, :Price, :Source, :itemcount, :uuid])
      DMMatcher.new(dm, properties)
    end

  end
end
