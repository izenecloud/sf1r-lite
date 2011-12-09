require 'rubygems'
$LOAD_PATH.unshift(File.dirname(__FILE__))
require 'sf1r_api'
require 'exporter/nothing_exporter.rb'
require 'exporter/collonly_exporter.rb'
require 'exporter/visit_exporter.rb'
require 'exporter/visit_item_exporter.rb'
require 'exporter/purchase_item_exporter.rb'

class CallHistroyExporter
  def initialize(api)
    @api = api
    @exporter_map = {}
    @exporter_map['documents'] = NothingExporter.new
    @exporter_map['documents/search'] = NothingExporter.new
    @exporter_map['recommend/add_user'] = NothingExporter.new
    @exporter_map['recommend/update_user'] = NothingExporter.new
    @exporter_map['recommend/remove_user'] = NothingExporter.new
    @exporter_map['documents/visit'] = VisitExporter.new("b5mm")
    @exporter_map['recommend/visit_item'] = VisitItemExporter.new("b5mm")
    @exporter_map['recommend/purchase_item'] = PurchaseItemExporter.new("b5mm")
#     @exporter_map['recommend/track_event'] = VisitConverter.new
#     @exporter_map['recommend/rate_item'] = VisitConverter.new
    
    
  end
  
  def convert(request)
    header = request["header"]
    return nil if header.nil?
    controller = header["controller"]
    action = header["action"]
    return nil if controller.nil?
    action_str = Sf1rApi::make_action_str(controller, action)
    exporter = @exporter_map[action_str]
    return nil if exporter.nil?
    exporter.convert(@api, request)
  end
  
end

    