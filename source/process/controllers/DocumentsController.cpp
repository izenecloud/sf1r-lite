/**
 * @file process/controllers/DocumentsController.cpp
 * @author Ian Yang
 * @date Created <2010-06-02 10:54:39>
 */

#include <bundles/index/IndexSearchService.h>
#include <bundles/mining/MiningSearchService.h>
 
#include "DocumentsController.h"
#include "DocumentsGetHandler.h"
#include "DocumentsSearchHandler.h"

#include <common/Keys.h>

namespace sf1r
{

using namespace izenelib::driver;
using driver::Keys;

const std::size_t DocumentsController::kDefaultPageCount = 20;

/**
 * @brief Action @b search. Gets documents from SF1 using full text search.
 *
 * @section request
 *
 * - @b collection* (@c String): Search in this collection.
 * - @b search* (@c Object): Full text search specification. See SearchParser.
 * - @b select (@c Array): Select properties in search result. See SelectParser.
 * - @b conditions (@c Array): Result filtering conditions. See
 *   driver::ConditionArrayParser.
 * - @b sort (@c Array): Sort result. See SortParser.
 * - @b group (@c Array): Group property values.
 *   - @b property* (@c String): Property name, whose group result (doc count for
 *     each property value) would be supplied in response["group"]. The property
 *     type must be string, int or float.
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
 * @brief Action @b get. Gets documents from SF1 by ids.
 *
 * @section request
 *
 * - @b collection* (@c String): Search in this collection.
 * - @b select (@c Array): Select properties in result. See SelectParser.
 * - @b search_session (@c Object): Trace last search. See SearchParser.
 * - @b conditions* (@c String): Get documents by filtering conditions. See
 *   driver::ConditionArrayParser, Now the action @b get only supports exactly
 *   one condition on property _id or DOCID using operator = or in.
 *   - _id: sf1 internal ID.
 *   - DOCID: Documents ID.
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
 * @brief Action @b create. Creates a documents in SIA and MIA though BA.
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
 *   "resource" => {
 *     "DOCID": "post.1",
 *     "title": "Hello, World",
 *     "content": "This is a test post."
 *   }
 * }
 * @endcode
 */
void DocumentsController::create()
{
    IZENELIB_DRIVER_BEFORE_HOOK(parseCollection());
    IZENELIB_DRIVER_BEFORE_HOOK(requireDOCID());
    bool requestSent = collectionHandler_->create(
            collection_, request()[Keys::resource]
    );

    if (!requestSent)
    {
        response().addError(
                "Failed to send request to given collection."
        );
        return;
    }
}

/**
 * @brief Action @b update. Updates documents in SIA and MIA though BA.
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
 *   "resource" => {
 *     "DOCID": "post.1",
 *     "title": "Hello, World (revision)",
 *     "content": "This is a revised test post."
 *   }
 * }
 * @endcode
 */
void DocumentsController::update()
{
    IZENELIB_DRIVER_BEFORE_HOOK(parseCollection());
    IZENELIB_DRIVER_BEFORE_HOOK(requireDOCID());
    bool requestSent = collectionHandler_->update(
        collection_, request()[Keys::resource]
    );

    if (!requestSent)
    {
        response().addError(
            "Failed to send request to given collection."
        );
        return;
    }
}

/**
 * @brief Action @b destroy. Destroys documents in SIA and MIA though BA.
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
 *   "resource" => {
 *     "DOCID": "post.1",
 *   }
 * }
 * @endcode
 */
void DocumentsController::destroy()
{
    IZENELIB_DRIVER_BEFORE_HOOK(parseCollection());
    IZENELIB_DRIVER_BEFORE_HOOK(requireDOCID());

    bool requestSent = collectionHandler_->destroy(
        collection_, request()[Keys::resource]
    );

    if (!requestSent)
    {
        response().addError(
            "Failed to send request to given collection."
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
 *   "resource" => {
 *     "DOCID": "docid-1",
 *   }
 * }
 * @endcode
 */
void DocumentsController::get_topic()
{
    IZENELIB_DRIVER_BEFORE_HOOK(requireDOCID());
  
    uint32_t internal_id = 0;
    Value& input = request()[Keys::resource];
    Value& docid_value = input[Keys::DOCID];
    std::string sdocid = asString(docid_value);
    izenelib::util::UString udocid(sdocid, izenelib::util::UString::UTF_8);
    bool success = collectionHandler_->indexSearchService_->getInternalDocumentId(udocid, internal_id);
    if (!success || internal_id==0)
    {
        response().addError("Cannot convert DOCID to internal ID");
        return;
    }
  
    std::vector<std::pair<uint32_t, izenelib::util::UString> > label_list;
    bool requestSent = collectionHandler_->miningSearchService_->getDocLabelList(
        internal_id, label_list
    );

    if (!requestSent)
    {
        response().addError(
            "Failed to send request to given collection."
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
 *   "resource" => {
 *     "DOCID": "docid-1",
 *   }
 * }
 * @endcode
 */
void DocumentsController::get_topic_with_sim()
{
    IZENELIB_DRIVER_BEFORE_HOOK(requireDOCID());
  
    uint32_t internal_id = 0;
    Value& input = request()[Keys::resource];
    Value& docid_value = input[Keys::DOCID];
    std::string sdocid = asString(docid_value);
    izenelib::util::UString udocid(sdocid, izenelib::util::UString::UTF_8);
    bool success = collectionHandler_->indexSearchService_->getInternalDocumentId(
          udocid, internal_id
      );
    if (!success || internal_id==0)
    {
        response().addError("Cannot convert DOCID to internal ID");
        return;
    }
  
    std::vector<std::pair<izenelib::util::UString, std::vector<izenelib::util::UString> > > label_list;
    bool requestSent = collectionHandler_->miningSearchService_->getLabelListWithSimByDocId(
        internal_id, label_list
    );

    if (!requestSent)
    {
        response().addError(
            "Failed to send request to given collection."
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

bool DocumentsController::parseCollection()
{
    collection_ = asString(request()[Keys::collection]);
    if (collection_.empty())
    {
        response().addError("Require field collection in request.");
        return false;
    }

    return true;
}

} // namespace sf1r

