/**
 * @file process/controllers/DocumentsController.cpp
 * @author Ian Yang
 * @date Created <2010-06-02 10:54:39>
 */

#include "DocumentsController.h"
#include "CollectionHandler.h"

#include <bundles/index/IndexSearchService.h>
#include <bundles/index/IndexTaskService.h>
#include <bundles/mining/MiningSearchService.h>

#include <common/Keys.h>
#include <common/Utilities.h>

namespace sf1r
{

using namespace izenelib::driver;
using driver::Keys;

const std::size_t DocumentsController::kDefaultPageCount = 20;

namespace
{

bool convertGroupLabel(
    const Value& pathValue,
    std::vector<std::string>& groupPath
)
{
    if (pathValue.type() != Value::kArrayType)
        return false;

    groupPath.resize(pathValue.size());

    for (std::size_t i = 0; i < pathValue.size(); ++i)
    {
        groupPath[i] = asString(pathValue(i));
    }

    return true;
}

}

DocumentsController::DocumentsController()
    : indexSearchService_(NULL)
    , indexTaskService_(NULL)
    , miningSearchService_(NULL)
{
}

bool DocumentsController::checkCollectionService(std::string& error)
{
    indexSearchService_ = collectionHandler_->indexSearchService_;
    if (! indexSearchService_)
    {
        error = "Request failed, no index search service found.";
        return false;
    }

    indexTaskService_ = collectionHandler_->indexTaskService_;
    if (! indexTaskService_)
    {
        error = "Request failed, no index task service found.";
        return false;
    }

    miningSearchService_ = collectionHandler_->miningSearchService_;
    if (! miningSearchService_)
    {
        error = "Request failed, no mining search service found.";
        return false;
    }
    return true;
}

/**
 * @brief Action @b search. Gets documents from SF1 using full text search.
 *
 * @section request
 *
 * - @b collection* (@c String): Search in this collection.
 * - @b search* (@c Object): Full text search specification. See SearchParser.
 * - @b select (@c Array): Select properties in search result. See SelectParser.
 * - @b conditions (@c Array): Result filtering conditions. See ConditionArrayParser.
 * - @b sort (@c Array): Sort result. See SortParser.
 * - @b group (@c Array): Group property values. See GroupingParser.
 * - @b range (@c Object): Property name for getting its property value range.
 *   - @b property* (@c String): Property name, whose value range (the maximum and
 *     minimum property value) would be supplied in response["range"]. The property
 *     type must be int or float.
 * - @b attr (@c Object): Options for group by attribute values
 *   - @b attr_result (@c Bool = @c false): If true, response["attr"] would be
 *     returned as search results grouped by attribute values.
 *   - @b attr_top (@c Uint = 0): Limit the array size in response["attr"].
 *     If zero, all attribute groups would be returned.
 *     Otherwise, at most @b attr_top elements would be returned in response["attr"].
 * - @b limit (@c Uint = 20): Limit the count of returned documents.
 * - @b offset (@c Uint = 0): Index offset of the first returned documents in
 *   all result. It is used together with @b limit in result paging. If every
 *   page contains 10 documents, then parameters @c "limit":10,"offset":0 is for
 *   the first page (documents with index 0 ~ 9 in all matched result), and @c
 *   "limit":10,"offset":30 is for the 4th page (documents with index 30 ~ 39 in
 *   all matched result).
 * - @b remove_duplicated_result (@c Bool = false): Whether remove duplicated
 *   documents from the result. It is \c false by default.
 * - @b mining_result (@c Bool = @c true): Whether return mining result.
 * - @b analyzer_result (@c Bool = @c false): Whether return property analyzer
 *   result. The result is returned as field "analyzer_result" in response.
 *
 * @section response
 *
 * - @b total_count (@c Uint): Total matched documents. Because result have been
 *   pruned, this value cannot be used in paging, use @b top_k_count instead.
 * - @b top_k_count (@c Uint): Count of documents in pruned top K result set. It
 *   should be used in paging for this API.
 * - @b resources (@c Array): Every item is a document Object. In the document
 *   Object, property name is the key, and property value is the value. There
 *   are some special properties may be in the result:
 *   - @b DOCID (@c String): Primary key specified by user in create().
 *   - @b _id (@c Uint): Internel id used by SF1 to identify the document.
 *   - @b _image_id (@c Uint): Internel image ID accompanying with the document.
 *   - @b _rank (@c Double): Ranking score of this document.
 *   - @b _duplicated_document_count (@c Uint): Number of documents duplicated
 *     with this document.
 *   - @b _similar_document_count (@c Uint): Number of documents similar to this
 *     document.
 *   - @b _categories (@c Array): Array of strings. Every string is a category
 *     name that this document belongs to.
 * - @b related_queries (@c Array): Array of strings. Every string is a related
 *   query.
 * - @b popular_queries (@c Array): Array of strings. Every string is a popular
 *   query.
 * - @b realtime_queries (@c Array): Array of strings. Every string is a
 *   realtime query.
 * - @b taxonomy (@c Object): Taxonomy of the result.
 *   - @b labels (@c Array): Taxonomy labels. Every item is an object
 *     representing a label. Labels are organized as a tree. The item has
 *     following fields.
 *     - @b label (@c String): Name of this label.
 *     - @b document_count (@c Uint): Number of result documents in this label.
 *     - @b sub_labels (@c Array): Array of sub labels. It has the same
 *       structure of the top @b labels field.
 * - @b range (@c Object): Property value range.
 *   - @b max (@c Float): Maximum value
 *   - @b min (@c Float): Minimum value
 * - @b count (@c Map): COUNT value map for Properties. Key refers to property name,
 *    value refers to COUNT value of this property.
 * - @b name_entity (@c Array): Every item represents one type of name
 *   entity. The item has following fields.
 *   - @b type (@c String): Name entity type.
 *   - @b name_entity_list (@c Array): Every item is a name entity item. The
 *     item has following fields.
 *     - @b name_entity_item (@c String): name entity item name.
 *     - @b document_support_count (@c Uint): How many result documents support
 *       this item.
 * - @b refined_query (@c String): Refined query if the result count is less
 *   than the threshold.
 * - @b group (@c Array): Every item represents the group result of one property.
 *   - @b property (@c String): Property name.
 *   - @b document_count (@c Uint): Number of result documents whose value on
 *     this property is not empty.
 *   - @b labels (@c Array): Group labels. Every item is an object
 *     representing a label for a property value. Labels are organized as a tree.
 *     The item has following fields.
 *     - @b label (@c String): Name of this label, it represents a property value.
 *     - @b document_count (@c Uint): Number of result documents in this label.
 *     - @b sub_labels (@c Array): Array of sub labels. It has the same
 *       structure of the top @b labels field.
 * - @b attr (@c Array): Every item represents the group result for one attribute.
 *   They are sorted by @b document_count decreasingly.
 *   - @b attr_name (@c String): Attribute name.
 *   - @b document_count (@c Uint): Number of result documents which has value on
 *     this attribute.
 *   - @b labels (@c Array): Group labels. Every item is an object representing
 *     a label for an attribute value. The item has following fields.
 *     - @b label (@c String): Name of this label, it represents an attribute value.
 *     - @b document_count (@c Uint): Number of result documents in this label.
 * - @b top_group_label (@c Array): Every item represents the auto selected group
 *   labels for one property.@n
 *   This would be returned only when @b request["search"]["group_label"]["value"]
 *   is an empty array.
 *   - @b group_property (@c String): Property name.
 *   - @b group_label (@c Array): An array of auto selected group labels.@n
 *     Each element is a group label, which is a @c String array of the path from
 *     root to leaf node.
 *
 * @section example
 *
 * Request
 * @include documents_index_request.json
 *
 * Response
 * @include documents_index_response.json
 */
void DocumentsController::search()
{
    IZENELIB_DRIVER_BEFORE_HOOK(setLimit());
    collectionHandler_->search(request(), response());
}

/**
 * @brief Action @b get. Gets documents from SF1 by ids (or property values).
 *
 * @section request
 *
 * - @b collection* (@c String): Search in this collection.
 * - @b select (@c Array): Select properties in result. See SelectParser.
 * - @b search_session (@c Object): Trace last search. See SearchParser.
 * - @b conditions* (@c String): Get documents by filtering conditions. See
 *   driver::ConditionArrayParser, Now the action @b get only supports exactly
 *   one condition on property _id or DOCID or <Property Name> using operator = or in.
 *   - _id: sf1 internal ID.
 *   - DOCID: Documents ID.
 *   - <Property Name>: which is a filterable property. (get docs by property value)
 *
 * @b search_session is required to highlight keywords in the result. Just copy
 * the @b search field from request message for "documents/search".
 *
 * @section response
 *
 * - @b resources (@c Array): Every item is a document Object. In the document
 *   Object, property name is the key, and property value is the value.
 *
 * @section example
 *
 * Request
 *
 * @code
 * {
 *   "collection": "ChnWiki",
 *   "conditions": [
 *     {"property": "id", "operator": "=", "value": 10}
 *   ],
 *   "select": [
 *     { "property": "title" },
 *     { "property": "content"}
 *   ]
 * }
 * @endcode
 *
 */
void DocumentsController::get()
{
    IZENELIB_DRIVER_BEFORE_HOOK(setLimit());
    collectionHandler_->get(request(), response());
}

/**
 * @brief Action @b similar_to. Gets documents similar to the specified
 * document.
 *
 * @section request
 *
 * - @b collection* (@c String): collection of the target document.
 * - @b select (@c Array): Select properties in result. See SelectParser.
 * - @b similar_to* (@c Object): Specify the target document. The Object have
 *   exactly one key _id (SF1 internal ID) or DOCID (User specified ID when the
 *   document is created though driver or SCD file)  and value is the ID of the
 *   corresponding ID type. @b similar_to can also be a @c String, then that
 *   String is assumed to be the DOCID of the target document.
 *   - @c {"similar_to":{"_id":10}} get documents similar to the one with
 *     internal id 10.
 *   - @c {"similar_to":{"DOCID":"post.1"}} get documents similar to the one
 *     with DOCID "post.1".
 *   - @c {"similar_to":"post.1"} identical with the second sample.
 * - @b limit (@c Uint = 20): Limit the count of returned documents.
 * - @b offset (@c Uint = 0): Index offset of the first returned documents in
 *   all result.
 * - @b search_session (@c Object): Trace last search. See SearchParser.
 *
 * search_session is required to highlight keywords in the result. Just copy
 * the @b search field from request message for "documents/search".
 *
 * @section response
 *
 * - @b resources (@c Array): Every item is a document Object. In the document
 *   Object, property name is the key, and property value is the value.
 *
 * @section example
 *
 * Request
 *
 * @code
 * {
 *   "collection": "ChnWiki",
 *   "similar_to": "1",
 *   "select": [
 *     { "property": "title" },
 *     { "property": "content"}
 *   ],
 *   "limit": 10
 * }
 * @endcode
 *
 */
void DocumentsController::similar_to()
{
    IZENELIB_DRIVER_BEFORE_HOOK(setLimit());
    collectionHandler_->similar_to(request(), response());
}

/**
 * @brief Action @b similar_to_image. Gets documents which accompanying images
 * are similar to the specified image.
 *
 * @section request
 *
 * - @b collection* (@c String): collection of the target document.
 * - @b select (@c Array): Select properties in result. See SelectParser.
 * - @b similar_to_image* (@c String): Specify the URI of the target image.
 * - @b limit (@c Uint = 20): Limit the count of returned documents.
 * - @b offset (@c Uint = 0): Index offset of the first returned documents in
 *   all result.
 * - @b search_session (@c Object): Trace last search. See SearchParser.
 *
 * search_session is required to highlight keywords in the result. Just copy
 * the @b search field from request message for "documents/search".
 *
 * @section response
 *
 * - @b resources (@c Array): Every item is a document Object. In the document
 *   Object, property name is the key, and property value is the value. The
 *   accompanying image ID is returned as property _image_id.
 *
 * @section example
 *
 * Request
 *
 * @code
 * {
 *   "collection": "ChnWiki",
 *   "similar_to_image": "http://img.izenesoft.cn/200K?p=3",
 *   "select": [
 *     { "property": "title" },
 *     { "property": "content"}
 *   ],
 *   "limit": 10
 * }
 * @endcode
 *
 */
void DocumentsController::similar_to_image()
{
    IZENELIB_DRIVER_BEFORE_HOOK(setLimit());
    collectionHandler_->similar_to_image(request(), response());
}

/**
 * @brief Action @b duplicate_with. Gets documents duplicate with the
 * specified document.
 *
 * @section request
 *
 * - @b collection* (@c String): collection of the target document.
 * - @b select (@c Array): Select properties in result. See SelectParser.
 * - @b duplicate_with* (@c Object): Specify the target document. It accepts
 *   the same parameter with @b similar_to in similar_to().
 * - @b limit (@c Uint = 20): Limit the count of returned documents.
 * - @b offset (@c Uint = 0): Index offset of the first returned documents in
 *   all result.
 * - @b search_session (@c Object): Trace last search. See SearchParser.
 *
 * search_session is required to highlight keywords in the result. Just copy
 * the @b search field from request message for "documents/search".
 *
 * @section response
 *
 * - @b resources (@c Array): Every item is a document Object. In the document
 *   Object, property name is the key, and property value is the value.
 *
 * @section example
 *
 * Request
 *
 * @code
 * {
 *   "collection": "ChnWiki",
 *   "duplicate_with": "1",
 *   "select": [
 *     { "property": "title" },
 *     { "property": "content"}
 *   ],
 *   "limit": 10
 * }
 * @endcode
 *
 */
void DocumentsController::duplicate_with()
{
    IZENELIB_DRIVER_BEFORE_HOOK(setLimit());
    collectionHandler_->duplicate_with(request(), response());
}

/**
 * @brief Action @b create. Create a document and write to SCD in specific collection, call index command to apply the changes.
 *
 * @section request
 *
 * - @b collection* (@c String): Create document in this collection.
 * - @b resource* (@c Object): A document resource. Property key name is used as
 *   key. The corresponding value is the content of the property. Property @b
 *   DOCID is required, which is a unique document identifier specified by
 *   client.
 *
 * @section response
 *
 * No extra fields.
 *
 * @section example
 *
 * @code
 * {
 *   "resource": {
 *     "DOCID": "post.1",
 *     "title": "Hello, World",
 *     "content": "This is a test post."
 *   }
 * }
 * @endcode
 */
void DocumentsController::create()
{
    IZENELIB_DRIVER_BEFORE_HOOK(requireDOCID());
    bool requestSent = collectionHandler_->create(
            request()[Keys::resource]
    );

    if (!requestSent)
    {
        response().addError(
                "Request Failed."
        );
        return;
    }
}

/**
 * @brief Action @b update. Update a document and write to SCD in specific collection, call index command to apply the changes.
 *
 * @section request
 *
 * - @b collection* (@c String): Update document in this collection.
 * - @b resource* (@c Object): A document resource. Property key name is used as
 *   key. The corresponding value is the content of the property. Property @b
 *   DOCID is required, which is a unique document identifier specified by
 *   client.
 *
 * @section response
 *
 * No extra fields.
 *
 * @section example
 *
 * @code
 * {
 *   "resource": {
 *     "DOCID": "post.1",
 *     "title": "Hello, World (revision)",
 *     "content": "This is a revised test post."
 *   }
 * }
 * @endcode
 */
void DocumentsController::update()
{
    IZENELIB_DRIVER_BEFORE_HOOK(requireDOCID());
    bool requestSent = collectionHandler_->update(
        request()[Keys::resource]
    );

    if (!requestSent)
    {
        response().addError(
            "Request Failed."
        );
        return;
    }
}

/**
 * @brief Action @b update in place. Update a document and write to SCD in specific collection, call index command to apply the changes.
 *
 * @section request
 *
 * - @b collection* (@c String): Update document in this collection.
 * - @b resource* (@c Object): specify the docid and the update operation.
 * - @b DOCID(@c String)  DOCID is required, which is a unique document identifier specified by
 *   client.
 * - @b update (@c Array): specify the detail update content.
 *   Each element is a update operation for the specify property.
 *   - @b op (@c String) What the operation will be done on the given property. 
 *   - @b property (@c String) property name that would apply the change. 
 *   - @b opvalue (@c String) the operation value.
 *
 * @section response
 *
 * - @b header (@c Object): Property @b success gives the result, true or false.
 *

 * @section response
 *
 * No extra fields.
 *
 * @section example
 *
 * Request
 *
 * @code
 * {
 *   "resource": {
 *     "DOCID": "post.1",
 *     "update": [
 *     { "op": "add/sub", "property":"price", "opvalue": "10" },
 *     { "op": "add/sub", "property":"count", "opvalue": "2" }
 *     ]
 *   }
 * }
 * @endcode
 */
void DocumentsController::update_inplace()
{
    IZENELIB_DRIVER_BEFORE_HOOK(requireDOCID());
    bool requestSent = collectionHandler_->update_inplace(
        request()[Keys::resource]
    );

    if (!requestSent)
    {
        response().addError(
            "Request Failed."
        );
        return;
    }
}


/**
 * @brief Action @b destroy. Destroy a document and write to SCD in specific collection, call index command to apply the changes.
 *
 * @section request
 *
 * - @b collection* (@c String): Destroy document in this collection.
 * - @b resource* (@c Object): Only field @b DOCID is used to find the document
 *   should be destroyed.
 *
 * @section response
 *
 * No extra fields.
 *
 * @section example
 *
 * @code
 * {
 *   "resource": {
 *     "DOCID": "post.1",
 *   }
 * }
 * @endcode
 */
void DocumentsController::destroy()
{
    IZENELIB_DRIVER_BEFORE_HOOK(requireDOCID());

    bool requestSent = collectionHandler_->destroy(
        request()[Keys::resource]
    );

    if (!requestSent)
    {
        response().addError(
            "Request Failed."
        );
        return;
    }
}

/**
 * @brief Action @b get_topic. Get topics of specific document
 *
 * @section request
 *
 * - @b collection* (@c String): Find topics in this collection.
 * - @b resource* (@c Object): Only field @b DOCID is used to find the document.
 *
 * @section response
 *
 * - @b resources (@c Array): Every item is a topic.
 *   - @b id (@c Uint): The id of topic
 *   - @b name (@c String): The name of topic
 *
 *
 * @section example
 *
 * @code
 * {
 *   "collection": "ChnWiki",
 *   "resource": {
 *     "DOCID": "docid-1",
 *   }
 * }
 * @endcode
 */
void DocumentsController::get_topic()
{
    IZENELIB_DRIVER_BEFORE_HOOK(requireDOCID());

    uint64_t internal_id = 0;
    Value& input = request()[Keys::resource];
    Value& docid_value = input[Keys::DOCID];
    std::string sdocid = asString(docid_value);

    bool success = indexSearchService_->getInternalDocumentId(collectionName_, Utilities::md5ToUint128(sdocid), internal_id);
    if (!success || internal_id==0)
    {
        response().addError("Cannot convert DOCID to internal ID");
        return;
    }

    std::vector<std::pair<uint32_t, izenelib::util::UString> > label_list;
    bool requestSent = miningSearchService_->getDocLabelList(
        internal_id, label_list
    );

    if (!requestSent)
    {
        response().addError(
            "Request Failed."
        );
        return;
    }
    Value& resources = response()[Keys::resources];
    resources.reset<Value::ArrayType>();
    for(uint32_t i=0;i<label_list.size();i++)
    {
      Value& new_resource = resources();
      new_resource[Keys::id] = label_list[i].first;
      std::string str;
      label_list[i].second.convertString(str, izenelib::util::UString::UTF_8);
      new_resource[Keys::name] = str;
    }
}

/**
 * @brief Action @b get_topic_with_sim. Get topics with their similar topics of specific document
 *
 * @section request
 *
 * - @b collection* (@c String): Find topics in this collection.
 * - @b resource* (@c Object): Only field @b DOCID is used to find the document.
 *
 * @section response
 *
 * - @b resources (@c Array): Every item is a topic.
 *   - @b name (@c String): The name of topic
 *   - @b sim_list (@c Array): The list of similar topics
 *     - @b name (@c String): The name of topic
 *
 *
 * @section example
 *
 * @code
 * {
 *   "collection": "ChnWiki",
 *   "resource": {
 *     "DOCID": "docid-1",
 *   }
 * }
 * @endcode
 */
void DocumentsController::get_topic_with_sim()
{
    IZENELIB_DRIVER_BEFORE_HOOK(requireDOCID());

    uint64_t internal_id = 0;
    Value& input = request()[Keys::resource];
    Value& docid_value = input[Keys::DOCID];
    std::string sdocid = asString(docid_value);
    bool success = indexSearchService_->getInternalDocumentId(collectionName_, Utilities::md5ToUint128(sdocid), internal_id);
    if (!success || internal_id==0)
    {
        response().addError("Cannot convert DOCID to internal ID");
        return;
    }

    std::vector<std::pair<izenelib::util::UString, std::vector<izenelib::util::UString> > > label_list;
    bool requestSent = miningSearchService_->getLabelListWithSimByDocId(
        internal_id, label_list
    );

    if (!requestSent)
    {
        response().addError(
            "Request Failed."
        );
        return;
    }
    Value& resources = response()[Keys::resources];
    resources.reset<Value::ArrayType>();
    for(uint32_t i=0;i<label_list.size();i++)
    {
      Value& new_resource = resources();
      std::string name;
      label_list[i].first.convertString(name, izenelib::util::UString::UTF_8);
      new_resource[Keys::name] = name;
      Value& sim_list = new_resource[Keys::sim_list];
      sim_list.reset<Value::ArrayType>();
      for(uint32_t j=0;j<label_list[i].second.size();j++)
      {
        Value& new_sim = sim_list();
        std::string str;
        label_list[i].second[j].convertString(str, izenelib::util::UString::UTF_8);
        new_sim[Keys::name] = str;
      }

    }
}

/**
 * @brief Action @b log_group_label. Log the group label click.@n
 * This log is used to increase the frequency count returned by @c get_freq_group_labels().
 *
 * @section request
 *
 * - @b collection* (@c String): collection name.
 * - @b resource* (@c Object): specify the keywords, property name and label value.
 *   - @b keywords* (@c String): user query
 *   - @b group_property* (@c String): the group property name, which property type must be string
 *   - @b group_label* (@c Array): the value of the group label. It's an array of the path from root to leaf node. Each is a @c String of node value.
 *
 * @section response
 *
 * - @b header (@c Object): Property @b success gives the result, true or false.
 *
 * @section example
 *
 * Request
 * @code
 * {
 *   "collection": "ChnWiki",
 *   "resource": {
 *     "keywords": "iphone4",
 *     "group_property": "Category",
 *     "group_label": ["手机数码", "手机通讯", "手机"]
 *   }
 * }
 * @endcode
 */
void DocumentsController::log_group_label()
{
    std::string query;
    std::string propName;
    GroupPath groupPath;

    if (requireKeywords(query) &&
        requireGroupProperty(propName) &&
        requireGroupLabel(groupPath))
    {
        if (! miningSearchService_->clickGroupLabel(
            query, propName, groupPath))
        {
            response().addError("Request Failed.");
        }
    }
}

/**
 * @brief Action @b get_freq_group_labels. Get frequently clicked group labels.@n
 *
 * @section request
 *
 * - @b collection* (@c String): collection name.
 * - @b resource* (@c Object): specify the keywords, property name, etc.
 *   - @b keywords* (@c String): user query
 *   - @b group_property* (@c String): the group property name, which property type must be string
 *   - @b limit (@c Uint = 1): Limit the count of returned labels
 *
 * @section response
 *
 * - @b resources (@c Array): Every item is a group label, sorted by @b freq decreasingly.@n
 *   Please note that if @c set_top_group_label() is called previously with a non-empty @b group_label,@n
 *   then those labels would be returned instead as @b group_label, and their @b freq would be zero.
 *   - @b freq (@c Uint): the frequency count
 *   - @b group_label (@c Array): the array of the path from root to leaf node, each is a @c String of node value
 *
 *
 * @section example
 *
 * Request
 * @code
 * {
 *   "collection": "ChnWiki",
 *   "resource": {
 *     "keywords": "iphone4",
 *     "group_property": "Category",
 *     "limit": "2"
 *   }
 * }
 * @endcode
 *
 * Response
 * @code
 * {
 *   "resources": [
 *     { "freq": 10,
 *       "group_label": ["手机数码", "手机通讯", "手机"] },
 *     { "freq": 5,
 *       "group_label": ["手机数码", "数码配件"] }
 *   ]
 * }
 * @endcode
 */
void DocumentsController::get_freq_group_labels()
{
    std::string query;
    std::string propName;

    if (!requireKeywords(query) ||
        !requireGroupProperty(propName))
    {
        return;
    }

    Value& input = request()[Keys::resource];
    int limit = asUintOr(input[Keys::limit], 1);

    std::vector<std::vector<std::string> > pathVec;
    std::vector<int> freqVec;
    if (! miningSearchService_->getFreqGroupLabel(
        query, propName, limit, pathVec, freqVec))
    {
        response().addError("Request Failed.");
        return;
    }

    Value& resources = response()[Keys::resources];
    resources.reset<Value::ArrayType>();
    for (std::size_t i = 0; i < pathVec.size(); ++i)
    {
        Value& new_resource = resources();
        new_resource[Keys::freq] = freqVec[i];

        Value& labelValue = new_resource[Keys::group_label];
        labelValue.reset<Value::ArrayType>();
        const std::vector<std::string>& path = pathVec[i];
        for (std::vector<std::string>::const_iterator it = path.begin();
            it != path.end(); ++it)
        {
            labelValue() = *it;
        }
    }
}

/**
 * @brief Action @b set_top_group_label. Set the most frequently clicked group label.
 *
 * @section request
 *
 * - @b collection* (@c String): collection name.
 * - @b resource* (@c Object): specify the keywords, property name and group labels.
 *   - @b keywords* (@c String): user query
 *   - @b group_property* (@c String): the group property name, which property type must be string
 *   - @b group_label* (@c Array): an array of group labels.@n
 *     Each element is a group label, which is a @c String array of the path from root to leaf node.@n
 *     If @b group_label is an empty array [], it would clear the top labels previously set.
 *
 * @section response
 *
 * - @b header (@c Object): Property @b success gives the result, true or false.
 *
 * @section example
 *
 * Request
 * @code
 * {
 *   "collection": "ChnWiki",
 *   "resource": {
 *     "keywords": "iphone4",
 *     "group_property": "Category",
 *     "group_label": [
 *       ["手机数码", "手机通讯", "手机"],
 *       ["手机数码", "手机通讯", "手机配件"]
 *    ]
 *   }
 * }
 * @endcode
 */
void DocumentsController::set_top_group_label()
{
    std::string query;
    std::string propName;
    std::vector<GroupPath> groupPathVec;

    if (requireKeywords(query) &&
        requireGroupProperty(propName) &&
        requireGroupLabelVec(groupPathVec))
    {
        if (! miningSearchService_->setTopGroupLabel(
            query, propName, groupPathVec))
        {
            response().addError("Request Failed.");
        }
    }
}

/**
 * @brief Action @b visit. Visit a spedified document, for updating click-through rate (CTR).
 *
 * @section request
 *
 * - @b collection* (@c String): Visit document in this collection.
 * - @b resource* (@c Object): Only field @b DOCID is used to find the document
 *   to be visited.
 *
 * @section response
 *
 * No extra fields.
 *
 * @section example
 *
 * @code
 * {
 *   "collection":"ChnWiki",
 *   "resource":
 *   {
 *     "DOCID":"12345",
 *     "USERID":"111",
 *     "session_id":"session_123",
 *     "context":
 *    {
 *      "query":"keywords",
 *      "rank":"bm25:desc",
 *      "pos":"2"
 *    }
 * }
 * @endcode
 */
void DocumentsController::visit()
{
    IZENELIB_DRIVER_BEFORE_HOOK(requireDOCID());

    uint64_t internalId = 0;
    Value& docidValue = request()[Keys::resource][Keys::DOCID];
    std::string docidStr = asString(docidValue);

    std::string context;///Not used
    Value& userIDValue = request()[Keys::resource][Keys::USERID];
    context = asString(userIDValue);
    Value& sessionValue = request()[Keys::resource][Keys::session_id];
    context = asString(sessionValue);
    Value& queryValue = request()[Keys::resource][Keys::context][Keys::query];
    context = asString(queryValue);
    Value& rankValue = request()[Keys::resource][Keys::context][Keys::rank];
    context = asString(rankValue);
    Value& posValue = request()[Keys::resource][Keys::context][Keys::pos];
    context = asString(posValue);

    bool need_get_id = true;
    if (request().callType() == Request::FromPrimaryWorker ||
        request().callType() == Request::FromLog)
    {
        need_get_id = false;
    }
    if (need_get_id && indexSearchService_->getInternalDocumentId(collectionName_, Utilities::md5ToUint128(docidStr), internalId)
            && internalId != 0)
    {
        if (!miningSearchService_->visitDoc(collectionName_, internalId))
        {
            response().addError("Failed to visit document");
        }
    }
    else if (!need_get_id)
    {
        if (!miningSearchService_->visitDoc(Utilities::md5ToUint128(docidStr)))
        {
            response().addError("Failed to visit document in distribute node.");
        }
    }
    else
    {
        response().addError("Cannot convert DOCID to internal ID");
    }
}

/**
 * @brief Action @b get_summarization. Get summarization according to specific identifiers
 *
 * @section request
 *
 * - @b collection* (@c String): Find topics in this collection.
 * - @b resource* (@c Object): Only field @b DOCID is used to find the document.
 *
 * @section response
 *
 * - @b resources (@c Array): Every item is an aspect of summarization.
 *   - @b aspect (@c String): The name of summary aspect
 *   - @b summary (@c Array): The content of the aspect summary, each item indicates a sub aspect
 *
 *
 * @section example
 *
 * @code
 * {
 *   "collection": "ChnWiki",
 *   "resource": {
 *     "DOCID": "docid-1",
 *   }
 * }
 * @endcode
 */
void DocumentsController::get_summarization()
{
    IZENELIB_DRIVER_BEFORE_HOOK(requireDOCID());
    Value& input = request()[Keys::resource];
    Value& docid_value = input[Keys::DOCID];
    std::string sdocid = asString(docid_value);
    izenelib::util::UString udocid(sdocid, izenelib::util::UString::UTF_8);

    Summarization result;
    bool success = miningSearchService_->GetSummarizationByRawKey(collectionName_, udocid, result);
    if (!success)
    {
        response().addError("Cannot get results for " + sdocid);
        return;
    }
    Value& resources = response()[Keys::resources];
    resources.reset<Value::ArrayType>();
    Summarization::property_const_iterator sIt = result.propertyBegin();
    for (; sIt != result.propertyEnd(); ++sIt)
    {
        Value& new_resource = resources();
        new_resource[Keys::aspect] = sIt->first;
        std::string str;
        Value& summary = new_resource[Keys::summary];
        for (uint32_t i = 0; i < sIt->second.size(); i++)
        {
            Value& summary_item = summary();
            sIt->second[i].second.convertString(str, izenelib::util::UString::UTF_8);
            summary_item[Keys::sentence] = str;
            summary_item[Keys::score] = sIt->second[i].first;
        }
    }
}


/**
 * @brief Action @b get_doc_count. Get indexed document count.
 *
 * @section request
 *
 * - @b collection* (@c String): Specify collection.
 *
 * @section response
 *
 * - @b count (@c Uint): Result count
 *
 *
 * @section example
 *
 * @code
 * {
 *   "collection": "b5ma"
 * }
 * @endcode
 */
void DocumentsController::get_doc_count()
{
    uint32_t doc_count = indexTaskService_->getDocNum();
    response()[Keys::count] = doc_count;
}

/**
 * @brief Action @b get_key_count. Get distinct property value count(as the search key).
 *
 * @section request
 *
 * - @b collection* (@c String): Specify collection.
 * - @b property* (@c String): Specify property name.
 *
 * @section response
 *
 * - @b count (@c Uint): Result count
 *
 *
 * @section example
 *
 * @code
 * {
 *   "collection": "b5ma",
 *   "property": "Source"
 * }
 * @endcode
 */
void DocumentsController::get_key_count()
{
    Value& pname = request()[Keys::property];
    std::string property_name = asString(pname);
    if(property_name.empty())
    {
        response().addError("Expect property name!");
        return;
    }
    uint32_t doc_count = indexTaskService_->getKeyCount(property_name);
    response()[Keys::count] = doc_count;
}

bool DocumentsController::requireDOCID()
{
    if (!request()[Keys::resource].hasKey(Keys::DOCID))
    {
        response().addError("Require property DOCID in document.");
        return false;
    }

    return true;
}

bool DocumentsController::setLimit()
{
    if (nullValue(request()[Keys::limit]))
    {
        request()[Keys::limit] = kDefaultPageCount;
    }

    return true;
}

bool DocumentsController::requireKeywords(std::string& keywords)
{
    Value& input = request()[Keys::resource];

    keywords = asString(input[Keys::keywords]);
    if (keywords.empty())
    {
        response().addError("Require resource[keywords] as user query.");
        return false;
    }

    return true;
}

bool DocumentsController::requireGroupProperty(std::string& groupProperty)
{
    Value& input = request()[Keys::resource];

    groupProperty = asString(input[Keys::group_property]);
    if (groupProperty.empty())
    {
        response().addError("Require resource[group_property] as group property name.");
        return false;
    }

    return true;
}

bool DocumentsController::requireGroupLabel(GroupPath& groupPath)
{
    Value& input = request()[Keys::resource];
    const Value& pathValue = input[Keys::group_label];

    if (! convertGroupLabel(pathValue, groupPath))
    {
        response().addError("Require resource[group_label] as an array of group path.");
        return false;
    }

    return true;
}

bool DocumentsController::requireGroupLabelVec(std::vector<GroupPath>& groupPathVec)
{
    Value& input = request()[Keys::resource];
    const Value& labelValue = input[Keys::group_label];

    if (labelValue.type() != Value::kArrayType)
    {
        response().addError("Require resource[group_label] as an array of group labels.");
        return false;
    }

    groupPathVec.resize(labelValue.size());

    for (std::size_t i = 0; i < labelValue.size(); ++i)
    {
        if (! convertGroupLabel(labelValue(i), groupPathVec[i]))
        {
            response().addError("Require resource[group_label][i] as an array of group path.");
            return false;
        }
    }

    return true;
}

} // namespace sf1r
