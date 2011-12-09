# $LOAD_PATH.unshift(File.dirname(__FILE__))
# require '../sf1r_api'
require File.expand_path(File.join(__FILE__, '../../sf1r_api.rb'))

class ExportHelper
  
  def self.get_uuid_search_call(mname, uuid)
    controller = "documents"
    action = "get"
    body = {}
    body['collection'] = mname
    body['conditions'] = []
    condition = {}
    condition['property'] = 'uuid'
    condition['operator'] = '='
    condition['value'] = []
    condition['value'][0] = uuid
    body['conditions'][0] = condition
    body['select'] = []
    selec = {}
    selec['property'] = "DOCID"
    body['select'][0] = selec
    return controller, action, body
  end
  
  def self.get_all_docid(api, mname, uuid)
    controller, action, body = ExportHelper::get_uuid_search_call(mname, uuid)
    response = api.send(controller, action, body)
    return nil if response['header'].nil?
    return nil if !response['header']['success']
    return nil if response['resources'].nil?
    docid_list = []
    response['resources'].each do |r|
      docid_list << r['DOCID']
    end
    return docid_list
  end
  
  def self.copy(obj)
    return Marshal.load(Marshal.dump(obj))
  end
  
end