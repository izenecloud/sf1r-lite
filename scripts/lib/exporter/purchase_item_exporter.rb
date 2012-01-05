require 'rubygems'
require 'require_all'
require_rel 'export_helper'
require_rel '../sf1r_api'

class PurchaseItemExporter
  def initialize(mname)
    @mname = mname
  end
  
  def convert(api, request)
#     puts 'PurchaseItemExporter convert'
    result = []
    request['resource']['items'].each do |item|
      docid_list = ExportHelper::get_all_docid(api, @mname, item['ITEMID'])
      next if docid_list.nil?
      docid_list.each do |docid|
        copyitem = ExportHelper::copy(item)
        copyitem['ITEMID'] = docid
        copy = ExportHelper::copy(request)
        copy['collection'] = @mname
        copy['resource']['items'].clear
        copy['resource']['items'] << copyitem
#         puts copy
        result << copy
      end
    end
    return result
  end
  
end
