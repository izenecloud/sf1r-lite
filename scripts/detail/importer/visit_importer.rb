require File.expand_path(File.join(__FILE__, '../../sf1r_api.rb'))
$LOAD_PATH.unshift(File.dirname(__FILE__))
require 'import_helper'

class VisitImporter
  def initialize(name, api)
    @name = name
    @api = api
    @map = Hash.new([])
  end
  
  def add_import(request)
    mname = request['collection']
    docid = request['resource']['DOCID']
    uuid = ImportHelper::get_uuid(@api, mname, docid)
    if uuid.nil?
      $stderr.puts "Get uuid error : #{docid}"
      return
    end
    @map[uuid] = [] if !@map.has_key? uuid
    @map[uuid] << docid
#     puts "map size : #{@map.length}"
#     puts "map : #{@map}"
#     if @map[uuid].size>1
#       puts "#{uuid} has #{@map[uuid].size} items"
#     end
  end
  
  def flush
    @map.each do |uuid, docid_list|
#       puts "uuid : #{uuid}"
      freq_map = docid_list.inject(Hash.new(0) ) { |h, v| h[v] += 1; h }
      max_freq_docid = docid_list.sort_by { |v| freq_map[v] }.last
      max_freq = freq_map[max_freq_docid]
#       puts "#{uuid} max_occur : #{max_freq}"
      controller, action, body = ImportHelper::get_visit_call(@name, uuid)
      max_freq.times do
        @api.send(controller, action, body)
      end
    end
  end
  
end