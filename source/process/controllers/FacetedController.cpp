#include "FacetedController.h"

#include <common/Keys.h>

#include <bundles/mining/MiningSearchService.h>

#include <boost/algorithm/string/split.hpp>

namespace sf1r
{

using driver::Keys;
using namespace izenelib::driver;

/**
 * @brief Action \b set_ontology. Sets ontology by xml format owl
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
 * - @b content (@c String): the owl content, @see https://ssl.izenesoft.cn/projects/sf1-revolution/wiki/Faceted_Search
 *
 * @section response
 *
 * Whether it is succ.
 *
 * @section Example
 *
 * Request
 * @code
 * {
 *   "collection": "ChnWiki",
 *   "content": "\<xml ....    \</xml\>",
 * }
 * @endcode
 *
 * Response
 * @code
 * {
 *   "header": {"success": true}
 * }
 * @endcode
 */
void FacetedController::set_ontology()
{
    MiningSearchService* service = collectionHandler_->miningSearchService_;

    bool requestSent = service->SetOntology(
                                    asString(request()[Keys::content])
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
 * @brief Action \b get_ontology. Gets ontology by xml format owl
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
 *
 * @section response
 *
 * - @b collection* (@c String): ontology owl xml
 *
 * @section Example
 *
 * Request
 * @code
 * {
 *   "collection": "ChnWiki"
 * }
 * @endcode
 *
 * Response
 * @code
 * {
 *   "header": {"success": true},
 *   "content": "\<xml ... \</xml\>"
 * }
 * @endcode
 */
void FacetedController::get_ontology()
{
    std::string xml;
    MiningSearchService* service = collectionHandler_->miningSearchService_;	
    bool requestSent = service->GetOntology(xml);

    if (!requestSent)
    {
        response().addError(
            "Failed to send request to given collection."
        );
        return;
    }
    response()[Keys::content] = xml;
}


/**
 * @brief Action \b get_static_rep. Gets static ontology representation
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
 *
 * @section response
 *
 * - @b resources (@c Array): Every item is a ontology category.
 *   - @b level (@c Uint): The depth in ontology tree, the top level is 1.
 *   - @b name (@c String): The name of category
 *   - @b id (@c Uint): The id of category
 *   - @b doccount (@c Uint): How many docs belong to this category, recursively.
 *
 * @section Example
 *
 * Request
 * @code
 * {
 *   "collection": "ChnWiki"
 * }
 * @endcode
 *
 * Response
 * @include faceted_rep_result.json
 */
void FacetedController::get_static_rep()
{
    faceted::OntologyRep rep;
    MiningSearchService* service = collectionHandler_->miningSearchService_;	
    bool requestSent =service->GetStaticOntologyRepresentation(rep);

    if (!requestSent)
    {
        response().addError(
            "Failed to send request to given collection."
        );
        return;
    }
    Value& resources = response()[Keys::resources];
    resources.reset<Value::ArrayType>();
    std::list<faceted::OntologyRepItem>::const_iterator it = rep.item_list.begin();
    while (it!=rep.item_list.end())
    {
        faceted::OntologyRepItem item = *it;
        ++it;
        std::string name;
        item.text.convertString(name, izenelib::util::UString::UTF_8);
        Value& new_resource = resources();
        new_resource[Keys::level] = item.level;
        new_resource[Keys::name] = name;
        new_resource[Keys::id] = item.id;
        new_resource[Keys::doccount] = item.doc_count;
    }

}


/**
 * @brief Action \b static_click. Clicks the category on specific collection
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
 * - @b category_id* (@c Uint): category id.
 *
 * @section response
 *
 * - @b resources (@c Array): Every item is a document _id
 *
 * @section Example
 *
 * Request
 * @code
 * {
 *   "collection": "ChnWiki",
 *   "category_id": "2"
 * }
 * @endcode
 *
 * Response
 * @code
 * {
 *   "header": {"success": true},
 *   "resources": [3,5,7]
 * }
 * @endcode
 */
void FacetedController::static_click()
{
    IZENELIB_DRIVER_BEFORE_HOOK(requireCID_());
    std::list<uint32_t> docid_list;
    MiningSearchService* service = collectionHandler_->miningSearchService_;	
    bool requestSent = service->OntologyStaticClick(cid_, docid_list);

    if (!requestSent)
    {
        response().addError(
            "Failed to send request to given collection."
        );
        return;
    }
    Value& resources = response()[Keys::resources];
    resources.reset<Value::ArrayType>();
    std::list<uint32_t>::const_iterator it = docid_list.begin();
    while (it!=docid_list.end())
    {
        uint32_t doc_id = *it;
        ++it;
        resources() = doc_id;
    }
}

/**
 * @brief Action \b get_rep.Get representation by search result
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
 * - @b docid_list* (@c String): Comma sperated search result docid list
 *
 * @section response
 *
 * - @b resources (@c Array): Every item is a ontology category.
 *   - @b level (@c Uint): The depth in ontology tree, the top level is 1.
 *   - @b name (@c String): The name of category
 *   - @b id (@c Uint): The id of category
 *   - @b doccount (@c Uint): How many docs belong to this category, recursively.
 *
 * @section Example
 *
 * Request
 * @code
 * {
 *   "collection": "ChnWiki",
 *   "docid_list": "1,2,3,6,7,10"
 * }
 * @endcode
 *
 * Response
 * @include faceted_rep_result.json
 */
void FacetedController::get_rep()
{
    IZENELIB_DRIVER_BEFORE_HOOK(requireSearchResult_());
    faceted::OntologyRep rep;
    MiningSearchService* service = collectionHandler_->miningSearchService_;	
    bool requestSent = service->GetOntologyRepresentation(search_result_, rep);

    if (!requestSent)
    {
        response().addError(
            "Failed to send request to given collection."
        );
        return;
    }
    Value& resources = response()[Keys::resources];
    resources.reset<Value::ArrayType>();
    std::list<faceted::OntologyRepItem>::const_iterator it = rep.item_list.begin();
    while (it!=rep.item_list.end())
    {
        faceted::OntologyRepItem item = *it;
        ++it;
        std::string name;
        item.text.convertString(name, izenelib::util::UString::UTF_8);
        Value& new_resource = resources();
        new_resource[Keys::level] = item.level;
        new_resource[Keys::name] = name;
        new_resource[Keys::id] = item.id;
        new_resource[Keys::doccount] = item.doc_count;
    }
}

/**
 * @brief Action \b click. Clicks the category in search result representation
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
 * - @b category_id* (@c Uint): category id.\
 * - @b docid_list* (@c String): Comma sperated search result docid list
 *
 * @section response
 *
 * - @b resources (@c Array): Every item is a document _id
 *
 * @section Example
 *
 * Request
 * @code
 * {
 *   "collection": "ChnWiki",
 *   "category_id": "2",
 *   "docid_list": "1,2,3,6,7,10"
 * }
 * @endcode
 *
 * Response
 * @code
 * {
 *   "header": {"success": true},
 *   "resources": [3,5,7]
 * }
 * @endcode
 */
void FacetedController::click()
{
    IZENELIB_DRIVER_BEFORE_HOOK(requireCID_());
    IZENELIB_DRIVER_BEFORE_HOOK(requireSearchResult_());
    std::list<uint32_t> docid_list;
    MiningSearchService* service = collectionHandler_->miningSearchService_;	
    bool requestSent = service->OntologyClick(search_result_, cid_, docid_list);
    if (!requestSent)
    {
        response().addError(
            "Failed to send request to given collection."
        );
        return;
    }
    Value& resources = response()[Keys::resources];
    resources.reset<Value::ArrayType>();
    std::list<uint32_t>::const_iterator it = docid_list.begin();
    while (it!=docid_list.end())
    {
        uint32_t doc_id = *it;
        ++it;
        resources() = doc_id;
    }
}

/**
 * @brief Action \b manmade. Man-made docid <-> category map
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
 * - @b manmade* (@c Array): Each item is a manmade docid to category map
 *   - @b _id (@c Uint): Document internal id
 *   - @b docid (@c String): The string docid
 *   - @b cid (@c Uint): The id of category
 *   - @b cname (@c String): category name
 *
 * @section response
 *
 * Whether it is succ
 *
 * @section Example
 *
 * Request
 * @code
 * {
 *   "collection": "ChnWiki",
 *   "manmade":
 *   [
 *     {
 *       "_id": 2,
 *       "docid": "DOC2",
 *       "cid": 2,
 *       "cname": "Sports"
 *     },
 *     {
 *       "_id": 5,
 *       "docid": "DOC5",
 *       "cid": 3,
 *       "cname": "Music"
 *     }
 *   ]
 * }
 * @endcode
 *
 * Response
 * @code
 * {
 *   "header": {"success": true},
 * }
 * @endcode
 */
void FacetedController::manmade()
{
    Value& manmades = request()[Keys::manmade];
    std::vector<faceted::ManmadeDocCategoryItem> items;
    while (true)
    {
        if (izenelib::driver::nullValue( manmades() )) break;
        faceted::ManmadeDocCategoryItem item;
        item.docid = asUint(manmades()[Keys::_id]);
        item.str_docid = asString(manmades()[Keys::docid]);
        item.cid = asUint(manmades()[Keys::cid]);
        item.cname = asString(manmades()[Keys::cname]);
        items.push_back(item);
    }
    MiningSearchService* service = collectionHandler_->miningSearchService_;	
    bool requestSent = service->DefineDocCategory(items);

    if (!requestSent)
    {
        response().addError(
            "Failed to send request to given collection."
        );
        return;
    }
}

bool FacetedController::requireCID_()
{
    if (!izenelib::driver::nullValue( request()[Keys::category_id] ) )
    {
        cid_ = asUint(request()[Keys::category_id]);
    }
    else
    {
        response().addError("Require field category_id in request.");
        return false;
    }

    return true;
}

bool FacetedController::requireSearchResult_()
{
    std::string str_sr = asString(request()[Keys::docid_list]);
    if (str_sr.empty())
    {
        response().addError("Require field docid_list in request.");
        return false;
    }
    std::vector<std::string> str_vec;
    boost::algorithm::split( str_vec, str_sr, boost::algorithm::is_any_of(",") );
    if (str_vec.empty())
    {
        response().addError("No valid search result in request.");
        return false;
    }
    search_result_.resize(str_vec.size());
    for (uint32_t i=0;i<str_vec.size();i++)
    {
        try
        {
            search_result_[i] = boost::lexical_cast<uint32_t>(str_vec[i]);
        }
        catch (boost::bad_lexical_cast& e)
        {
            response().addError(e.what());
            return false;
        }
    }
    return true;
}

}

