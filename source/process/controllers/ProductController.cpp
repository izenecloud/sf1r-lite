#include <bundles/product/ProductSearchService.h>
#include "ProductController.h"
#include <product-manager/product_manager.h>
#include <common/Keys.h>
#include <util/ustring/UString.h>
#include <vector>
#include <string>
#include <boost/unordered_set.hpp>

namespace sf1r
{

using driver::Keys;
using namespace izenelib::driver;

bool ProductController::check_product_manager_()
{
    if(collectionHandler_->productSearchService_==NULL)
    {
        response().addError("ProductManager not enabled.");
        return false;
    }
    product_manager_ = collectionHandler_->productSearchService_->GetProductManager();

    if(!product_manager_)
    {
        response().addError("ProductManager not enabled.");
        return false;
    }
    return true;
}

bool ProductController::require_docs_()
{
    Value& resources = request()[Keys::docid_list];
    for(uint32_t i=0;i<resources.size();i++)
    {
        Value& resource = resources(i);
        uint32_t docid = asUint(resource);
        docid_list_.push_back(docid);
    }
    if (docid_list_.empty())
    {
        response().addError("Require docid_list in request.");
        return false;
    }
    return true;
}

bool ProductController::require_uuid_()
{
    Value& vuuid = request()[Keys::uuid];
    std::string suuid = asString(vuuid);
    if(suuid.empty())
    {
        response().addError("Require uuid in request.");
        return false;
    }
    else
    {
        uuid_ = izenelib::util::UString(suuid, izenelib::util::UString::UTF_8);
    }
    return true;
}




/**
 * @brief Action \b add_new_group. Use new documents id list to generate a new group
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
 * - @b docid_list* (@c Array): document id list
 *
 * @section response
 *
 * - @b uuid (@c String): Generated uuid.
 * 
 *
 * @section Example
 *
 * Request
 * @code
 * {
 *   "collection" : "intelm",
 *   "docid_list": [5,6,7,8]
 * }
 * @endcode
 *
 * Response
 * @code
 * {
 *   "header": {"success": true},
 *   "uuid" : "xxx-yyy-zzz"
 * }
 * @endcode
 */
void ProductController::add_new_group()
{
    IZENELIB_DRIVER_BEFORE_HOOK(check_product_manager_());
    IZENELIB_DRIVER_BEFORE_HOOK(require_docs_());
    izenelib::util::UString uuid;
    if(!product_manager_->AddGroup(docid_list_, uuid))
    {
        response().addError(product_manager_->GetLastError());
        return;
    }
    std::string suuid;
    uuid.convertString(suuid, izenelib::util::UString::UTF_8);
    response()[Keys::uuid] = suuid;
}

/**
 * @brief Action \b append_to_group. Append new documents id list to an exist group
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
 * - @b uuid*       (@c String): uuid
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
 *   "collection" : "intelm",
 *   "uuid" : "xxx-yyy-zzz",
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
void ProductController::append_to_group()
{
    IZENELIB_DRIVER_BEFORE_HOOK(check_product_manager_());
    IZENELIB_DRIVER_BEFORE_HOOK(require_uuid_());
    IZENELIB_DRIVER_BEFORE_HOOK(require_docs_());
    if(!product_manager_->AppendToGroup(uuid_, docid_list_))
    {
        response().addError(product_manager_->GetLastError());
        return;
    }
}

/**
 * @brief Action \b remove_from_group. Remove documents id list from an exist group
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
 * - @b uuid*       (@c String): uuid
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
 *   "collection" : "intelm",
 *   "uuid" : "xxx-yyy-zzz",
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
void ProductController::remove_from_group()
{
    IZENELIB_DRIVER_BEFORE_HOOK(check_product_manager_());
    IZENELIB_DRIVER_BEFORE_HOOK(require_uuid_());
    IZENELIB_DRIVER_BEFORE_HOOK(require_docs_());
    if(!product_manager_->RemoveFromGroup(uuid_, docid_list_))
    {
        response().addError(product_manager_->GetLastError());
        return;
    }
}

    
}
