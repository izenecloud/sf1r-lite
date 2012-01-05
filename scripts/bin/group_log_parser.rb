# convert documents API from search() to log_group_label()

require 'rubygems'
require 'json'
require 'set'

class GroupLogParser
  def parse(request_str)
    reset
    extract(request_str) rescue nil
    empty? ? nil : result
  end

private
  FILTER = Set["documents/", "documents/search"]

  def reset
    @collection = @keywords = @property = @label = nil
  end

  # may throw exception if error happen
  def extract(request_str)
    request = JSON.parse(request_str)

    header = request["header"]
    controller = header["controller"]
    action = header["action"]
    uri = "#{controller}/#{action}"
    search = request["search"]

    return unless FILTER.include?(uri) && search["log_group_labels"]

    group_label_array = search["group_label"]
    group_label = group_label_array[0]

    @collection = request["collection"]
    @keywords = search["keywords"]
    @property = group_label["property"]
    @label = group_label["value"]
  end

  def empty?
    for obj in [@collection, @keywords, @property, @label]
      return true if obj.nil? || obj.empty?
    end

    false
  end

  def result
    request = {
      "header" => {
        "controller" => "documents",
        "action" => "log_group_label"
      },
      "collection" => @collection,
      "resource" => {
        "keywords" => @keywords,
        "group_property" => @property,
        "group_label" => @label
      }
    }

    request.to_json.to_s
  end
end
