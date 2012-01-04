require 'rubygems'
require 'require_all'
require_rel 'export_helper'
require_rel '../sf1r_api'

#nothing but convert collection name
class NothingExporter
  def initialize
  end
  
  def convert(api, request)
    copy = ExportHelper::copy(request)
    return copy
  end
  
end
