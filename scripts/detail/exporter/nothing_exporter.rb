require 'rubygems'
$LOAD_PATH.unshift(File.dirname(__FILE__))
require 'export_helper'
# require '../sf1r_api'

#nothing but convert collection name
class NothingExporter
  def initialize
  end
  
  def convert(api, request)
    copy = ExportHelper::copy(request)
    return copy
  end
  
end