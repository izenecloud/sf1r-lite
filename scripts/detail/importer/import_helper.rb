# $LOAD_PATH.unshift(File.dirname(__FILE__))
# require '../sf1r_api'
require File.expand_path(File.join(__FILE__, '../../sf1r_api.rb'))

class ImportHelper
  
  def self.get_docid_search_call(name, docid)
    controller = "documents"
    action = "get"
    body = {}
    body['collection'] = name
    body['conditions'] = []
    condition = {}
    condition['property'] = 'DOCID'
    condition['operator'] = '='
    condition['value'] = []
    condition['value'][0] = docid
    body['conditions'][0] = condition
    body['select'] = []
    selec = {}
    selec['property'] = "uuid"
    body['select'][0] = selec
    return controller, action, body
  end
  
  def self.get_uuid(api, name, docid)
    controller, action, body = ImportHelper::get_docid_search_call(name, docid)
    response = api.send(controller, action, body)
    return nil if response['header'].nil?
    return nil if !response['header']['success']
    return nil if response['resources'].nil?
    
    return nil if response['resources'].size!=1
    return response['resources'][0]['uuid']
  end
  
  def self.copy(obj)
    return Marshal.load(Marshal.dump(obj))
  end
  
  def self.get_visit_call(name, docid)
    controller = "documents"
    action = "visit"
    body = {}
    body['collection'] = name
    body['resource'] = {}
    body['resource']['DOCID'] = docid
    return controller, action, body
  end
  
  def self.get_visit_item_call(name, id)
    controller = "recommend"
    action = "visit_item"
    body = {}
    body['collection'] = name
    body['resource'] = {}
    body['resource']['is_recommend_item'] = "false"
    body['resource']['ITEMID'] = id[0]
    body['resource']['USERID'] = id[1]
    body['resource']['session_id'] = id[2]
    return controller, action, body
  end
  
  def self.get_purchase_item_call(name, id)
    controller = "recommend"
    action = "purchase_item"
    body = {}
    body['collection'] = name
    body['resource'] = {}
    body['resource']['USERID'] = id[1]
    body['resource']['items'] = []
    item = {}
    item['ITEMID'] = id[0]
    item['price'] = id[2]
    item['quantity'] = id[3]
    body['resource']['items'][0] = item
    return controller, action, body
  end
  
end