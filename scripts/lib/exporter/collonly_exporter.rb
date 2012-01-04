require 'rubygems'
require 'require_all'
require_rel 'export_helper'
# require '../sf1r_api'

#nothing but convert collection name
class CollonlyExporter
  def initialize(mname)
    @mname = mname
  end
  
  def convert(api, request)
    copy = ExportHelper::copy(request)
    copy['collection'] = @mname
    return copy
  end
  
end
