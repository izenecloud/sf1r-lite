require 'rubygems'
$LOAD_PATH.unshift(File.dirname(__FILE__))
require 'sf1r_api'
require 'importer/direct_importer.rb'
require 'importer/visit_importer.rb'
require 'importer/visit_item_importer.rb'
require 'importer/purchase_item_importer.rb'

class BackupHistroyImporter
  def initialize(api)
    @api = api
    @importer_map = {}
    #@importer_map['documents'] = DirectImporter.new(@api)
    #@importer_map['documents/search'] = DirectImporter.new(@api)
    @importer_map['documents/log_group_label'] = DirectImporter.new(@api)
    @importer_map['recommend/add_user'] = DirectImporter.new(@api)
    @importer_map['recommend/update_user'] = DirectImporter.new(@api)
    @importer_map['recommend/remove_user'] = DirectImporter.new(@api)
    @importer_map['documents/visit'] = VisitImporter.new("b5ma", @api)
    @importer_map['recommend/visit_item'] = VisitItemImporter.new("b5ma", @api)
    @importer_map['recommend/purchase_item'] = PurchaseItemImporter.new("b5ma", @api)
#     @importer_map['recommend/track_event'] = VisitConverter.new
#     @importer_map['recommend/rate_item'] = VisitConverter.new
    
    
  end
  
  def add_import(request)
    header = request["header"]
    return nil if header.nil?
    controller = header["controller"]
    action = header["action"]
    return nil if controller.nil?
    action_str = Sf1rApi::make_action_str(controller, action)
    importer = @importer_map[action_str]
    return nil if importer.nil?
    importer.add_import(request)
  end
  
  def flush
    @importer_map.each do |key, value|
      value.flush
    end
  end
  
end

    
