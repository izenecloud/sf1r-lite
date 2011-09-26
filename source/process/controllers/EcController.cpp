#include "EcController.h"
#include <mining-manager/ec-submanager/ec_manager.h>
#include <common/Keys.h>
#include <util/ustring/UString.h>
#include <vector>
#include <string>
#include <boost/unordered_set.hpp>

namespace sf1r
{

using driver::Keys;
using namespace izenelib::driver;

bool EcController::check_ec_manager_()
{
    boost::shared_ptr<MiningManager> mining_manager = GetMiningManager();

    ec_manager_ = mining_manager->GetEcManager();
    if(!ec_manager_)
    {
        response().addError("Ec not enabled.");
        return false;
    }
    return true;
}

bool EcController::require_tid_()
{
    if (!izenelib::driver::nullValue( request()[Keys::tid] ) )
    {
        tid_ = asUint(request()[Keys::tid]);
    }
    else
    {
        response().addError("Require field tid in request.");
        return false;
    }

    return true;
}

bool EcController::require_docs_()
{
    Value& resources = request()[Keys::docid_list];
    std::vector<std::pair<izenelib::util::UString,izenelib::util::UString> > input;
    for(uint32_t i=0;i<resources.size();i++)
    {
        Value& resource = resources(i);
        uint32_t docid = asUint(resource[Keys::query]);
        docid_list_.push_back(docid);
    }
    if (docid_list_.empty())
    {
        response().addError("Require docid_list in request.");
        return false;
    }
    return true;
}

/**
 * @brief Action \b get_all_tid. Get all exist tids.
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
 * - @b tid* (@c Uint): specify tid
 *
 * @section response
 *
 * - @b resources (@c Array): Every item is a tid.
 * 
 *
 * @section Example
 *
 * Request
 * @code
 * {
 *   "collection" : "intel",
 *   "tid": 1
 * }
 * @endcode
 *
 * Response
 * @code
 * {
 *   "header": {"success": true},
 *   "resources" : [3,4]
 * }
 * @endcode
 */
void EcController::get_all_tid()
{
    IZENELIB_DRIVER_BEFORE_HOOK(check_ec_manager_());
    std::vector<uint32_t> all_tids;
    if(!ec_manager_->GetAllGroupIdList(all_tids))
    {
        response().addError("GetAllGroupIdList failed.");
        return;
    }
    Value& resources = response()[Keys::resources];
    resources.reset<Value::ArrayType>();
    for(uint32_t i=0;i<all_tids.size();i++)
    {
        resources() = all_tids[i];
    }
}


/**
 * @brief Action \b get_docs_by_tid. Get documents id by tid.
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
 * - @b tid* (@c Uint): specify tid
 *
 * @section response
 *
 * - @b resources (@c Array): Every item is a document id.
 * 
 *
 * @section Example
 *
 * Request
 * @code
 * {
 *   "collection" : "intel",
 *   "tid": 1
 * }
 * @endcode
 *
 * Response
 * @code
 * {
 *   "header": {"success": true},
 *   "resources" : [3,4]
 * }
 * @endcode
 */
void EcController::get_docs_by_tid()
{
    IZENELIB_DRIVER_BEFORE_HOOK(check_ec_manager_());
    IZENELIB_DRIVER_BEFORE_HOOK(require_tid_());
    std::vector<uint32_t> docid_list;
    std::vector<uint32_t> candidate_list;
    if(!ec_manager_->GetGroupAll(tid_, docid_list, candidate_list))
    {
        response().addError("tid not exists.");
        return;
    }
    Value& resources = response()[Keys::resources];
    Value& docs = resources()[Keys::docid_list];
    for(uint32_t i=0;i<docid_list.size();i++)
    {
        docs() = docid_list[i];
    }
    Value& candidate = resources()[Keys::candidate_list];
    for(uint32_t i=0;i<candidate_list.size();i++)
    {
        candidate() = candidate_list[i];
    }
}


/**
 * @brief Action \b get_tids_by_docs. Get tid list by documents id list.
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
 * - @b docid_list* (@c Array): document id list
 *
 * @section response
 *
 * - @b resources (@c Array): Every item is a tid.
 * 
 *
 * @section Example
 *
 * Request
 * @code
 * {
 *   "collection" : "intel",
 *   "docid_list": [5,6,7,8]
 * }
 * @endcode
 *
 * Response
 * @code
 * {
 *   "header": {"success": true},
 *   "resources" : [3,4]
 * }
 * @endcode
 */
void EcController::get_tids_by_docs()
{
    IZENELIB_DRIVER_BEFORE_HOOK(check_ec_manager_());
    IZENELIB_DRIVER_BEFORE_HOOK(require_docs_());
    boost::unordered_set<uint32_t> tid_set;
    for(uint32_t i=0;i<docid_list_.size();i++)
    {
        uint32_t tid = 0;
        if(ec_manager_->GetGroupId(docid_list_[i], tid))
        {
            tid_set.insert(tid);
        }
    }
    std::vector<uint32_t> result;
    boost::unordered_set<uint32_t>::iterator it = tid_set.begin();
    while(it!=tid_set.end())
    {
        result.push_back(*it);
        ++it;
    }
    std::sort(result.begin(), result.end());
    Value& resources = response()[Keys::resources];
    for(uint32_t i=0;i<result.size();i++)
    {
        resources() = result[i];
    }
}


/**
 * @brief Action \b add_new_tid. Use new documents id list to generate a new tid group
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
 * - @b docid_list* (@c Array): document id list
 *
 * @section response
 *
 * - @b tid (@c Uint): Generated tid
 * 
 *
 * @section Example
 *
 * Request
 * @code
 * {
 *   "collection" : "intel",
 *   "docid_list": [5,6,7,8]
 * }
 * @endcode
 *
 * Response
 * @code
 * {
 *   "header": {"success": true},
 *   "tid" : 1
 * }
 * @endcode
 */
void EcController::add_new_tid()
{
    IZENELIB_DRIVER_BEFORE_HOOK(check_ec_manager_());
    IZENELIB_DRIVER_BEFORE_HOOK(require_docs_());
    std::vector<uint32_t> candidate_list;
    uint32_t tid = 0;
    if(!ec_manager_->AddNewGroup(docid_list_, candidate_list, tid))
    {
        response().addError("AddNewGroup failed.");
        return;
    }
    response()[Keys::tid] = tid;
}


/**
 * @brief Action \b add_docs_to_tid. Add documents id list to exist tid group
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
 * - @b tid* (@c Uint): tid.
 * - @b docid_list* (@c Array): document id list
 *
 * @section response
 *
 * 
 *
 * @section Example
 *
 * Request
 * @code
 * {
 *   "collection" : "intel",
 *   "tid" : 1,
 *   "docid_list": [5,6,7,8]
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
void EcController::add_docs_to_tid()
{
    IZENELIB_DRIVER_BEFORE_HOOK(check_ec_manager_());
    IZENELIB_DRIVER_BEFORE_HOOK(require_tid_());
    IZENELIB_DRIVER_BEFORE_HOOK(require_docs_());
    if(!ec_manager_->AddDocInGroup(tid_, docid_list_))
    {
        response().addError("AddDocInGroup failed.");
        return;
    }
}

/**
 * @brief Action \b remove_docs_from_tid. Remove documents id list from exist tid group
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
 * - @b tid* (@c Uint): tid.
 * - @b docid_list* (@c Array): document id list
 *
 * @section response
 *
 * 
 *
 * @section Example
 *
 * Request
 * @code
 * {
 *   "collection" : "intel",
 *   "tid" : 1,
 *   "docid_list": [5,6,7,8]
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
void EcController::remove_docs_from_tid()
{
    IZENELIB_DRIVER_BEFORE_HOOK(check_ec_manager_());
    IZENELIB_DRIVER_BEFORE_HOOK(require_tid_());
    IZENELIB_DRIVER_BEFORE_HOOK(require_docs_());
    if(!ec_manager_->RemoveDocInGroup(tid_, docid_list_))
    {
        response().addError("RemoveDocInGroup failed.");
        return;
    }


}

void EcController::get_all_product_info()
{
    IZENELIB_DRIVER_BEFORE_HOOK(check_ec_manager_());
    uint32_t categories = 0, products = 0, products_in_groups = 0, count = 0;
    std::vector<uint32_t> all_tids;
    if(!ec_manager_->GetAllGroupIdList(all_tids))
    {
        response().addError("GetAllGroupIdList failed.");
        return;
    }

    boost::shared_ptr<MiningManager> mining_manager = GetMiningManager();
    boost::shared_ptr<DocumentManager> document_manager = mining_manager->GetDocumentManager();
    std::string source_field = GetProductSourceField();

    for( size_t i = 0; i < all_tids.size(); i++ )
    {
        std::vector<uint32_t> docid_list;
        std::vector<uint32_t> candidate_list;
        ec_manager_->GetGroupAll(all_tids[i], docid_list, candidate_list);
        set<std::string> product_count;
        for(size_t j = 0; j < docid_list.size(); j++)
        {
            PropertyValue value;
            if ( !source_field.empty()
                    && document_manager->getPropertyValue(docid_list[j], source_field, value) )
            {
                izenelib::util::UString sourceFieldValue = get<izenelib::util::UString>(value);
                std::string strValue("");
                sourceFieldValue.convertString(strValue, izenelib::util::UString::UTF_8);
                product_count.insert(strValue);
            }
        }

        if(source_field.empty())
        {
            count += docid_list.size();
        }
        else
        {
            count += product_count.size();
        }
        products_in_groups += docid_list.size();
    }

    categories = GetDocNum() - products_in_groups + all_tids.size();
    products = GetDocNum() - products_in_groups + count;
    Value& productsInfo = response()[Keys::resource];
    productsInfo[Keys::categories] = categories;
    productsInfo[Keys::products] = products;
}
    
}
