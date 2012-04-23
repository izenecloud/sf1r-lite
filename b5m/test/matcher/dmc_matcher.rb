require "rspec/matchers"
require "builder"

module RSpec
  module Matchers
    class DMCMatcher

      def initialize(dmo)
        @dmo = dmo
        @error = ""
      end

      def matches?(dmc)
        dmc.docs.each do |cdoc|
          next if cdoc.nil?
          cid = cdoc[:DOCID]
          oid = cdoc[:ProdDocid]
          pid = cdoc[:uuid]
          odoc = @dmo.get(oid)
          if odoc.nil?
            if pid!=""
              @error = "can not get doc in dmo : #{oid}, cid:#{cid}, pid:#{pid}"
              return false
            end
          elsif pid!=odoc[:uuid]
            @error = "pid not the same, dmc - dmo : #{pid} - #{odoc[:uuid]}"
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

    def dmc_match(dmo)
      DMCMatcher.new(dmo)
    end

  end
end
