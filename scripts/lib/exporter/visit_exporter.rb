require 'rubygems'
require 'require_all'
require_rel 'export_helper'
require_rel '../sf1r_api'

class VisitExporter
  def initialize(mname)
    @mname = mname
  end
  
  def convert(api, request)
    uuid = request['resource']['DOCID'] 
    docid_list = ExportHelper::get_all_docid(api, @mname, uuid)
    return nil if docid_list.nil?
#     puts docid_list
    result = []
    docid_list.each do |docid|
      copy = ExportHelper::copy(request)
      copy['resource']['DOCID'] = docid
      copy['collection'] = @mname
#       puts copy
      result << copy
    end
    return result
  end
  
end
