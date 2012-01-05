require 'rubygems'
require 'require_all'
require_rel '../sf1r_api.rb'

class DirectImporter
  def initialize(api)
    @api = api
  end
  
  def add_import(request)
    @api.send_request(request, false)
  end
  
  def flush
  end
  
end
