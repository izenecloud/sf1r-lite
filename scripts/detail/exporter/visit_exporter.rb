require 'rubygems'
$LOAD_PATH.unshift(File.dirname(__FILE__))
require 'export_helper'
# require '../sf1r_api'

class VisitExporter
  def initialize(mname)
    @mname = mname
  end
  
  def convert(api, request)
    docid_list = ExportHelper::get_all_docid(api, @mname, request['resource']['DOCID'])
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