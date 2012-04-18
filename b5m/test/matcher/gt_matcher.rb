require "rspec/matchers"
require "builder"

module RSpec
  module Matchers
    class GTMatcher

      def initialize(gt)
        @gt = gt
        @error = ""
      end
      
      def matches?(another_gt)
        map = @gt.map
        another_map = another_gt.map
        if map.size!=another_map.size
          @error = "map size in caller and instance #{another_map.size} - #{map.size}"
          return false
        end
        if map.size==0
          return true
        end

        fmap = {}
        map.each_pair do |docid,gid|
          fmap[gid] ||= []
          fmap[gid] << docid
        end

        another_fmap = {}
        another_map.each_pair do |docid,gid|
          another_fmap[gid] ||= []
          another_fmap[gid] << docid
        end

        processed = {}
        map.each_pair do |docid,gid|
          next if processed.has_key?(gid)
          another_gid = another_map[docid]
          if another_gid.nil?
            @error = "caller does not have docid #{docid}"
            return false
          end
          docid_list = fmap[gid]
          another_docid_list = another_fmap[another_gid]
          docid_list.sort!
          another_docid_list.sort!
          if docid_list!=another_docid_list
            @error = "docid_list does not match for docid #{docid} : #{another_docid_list} | #{docid_list}"
            return false
          end
          processed[gid] = 1
        end

        true
      end

      def failure_message_for_should
        "#{@error}"
      end

      private

    end

    def gt_equal(gt)
      GTMatcher.new(gt)
    end

  end
end
