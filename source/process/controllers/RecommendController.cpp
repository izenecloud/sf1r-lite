/**
 * @file process/controllers/RecommendController.cpp
 * @author Jun Jiang
 * @date Created <2011-04-21>
 */
#include "RecommendController.h"
#include <bundles/recommend/RecommendTaskService.h>
#include <bundles/recommend/RecommendSearchService.h>
#include <recommend-manager/User.h>
#include <recommend-manager/Item.h>

#include <common/Keys.h>

namespace
{
/// @brief default count of recommended items in each request.
const std::size_t kDefaultRecommendCount = 10;

const izenelib::util::UString::EncodingType kEncoding = izenelib::util::UString::UTF_8;
}

namespace sf1r
{

using namespace izenelib::driver;
using driver::Keys;

bool RecommendController::requireProperty(const std::string& propName)
{
    if (!request()[Keys::resource].hasKey(propName))
    {
        response().addError("Require property " + propName + " in request[resource].");
        return false;
    }

    return true;
}

bool RecommendController::value2User(const izenelib::driver::Value& value, User& user)
{
    const Value::ObjectType& objectValue = value.getObject();

    const RecommendSchema& recommendSchema = collectionHandler_->recommendSchema_;	
    RecommendProperty recommendProperty;
    for (Value::ObjectType::const_iterator it = objectValue.begin();
        it != objectValue.end(); ++it)
    {
        const std::string& propName = it->first;
        if (propName == Keys::USERID)
        {
            user.idStr_ = asString(it->second);
        }
        else
        {
            if (recommendSchema.getUserProperty(propName, recommendProperty))
            {
                user.propValueMap_[propName].assign(
                    asString(it->second),
                    kEncoding
                );
            }
            else
            {
                response().addError("Unknown user property " + propName + " in request[resource].");
                return false;
            }
        }
    }

    return true;
}

bool RecommendController::value2Item(const izenelib::driver::Value& value, Item& item)
{
    const Value::ObjectType& objectValue = value.getObject();

    const RecommendSchema& recommendSchema = collectionHandler_->recommendSchema_;	
    RecommendProperty recommendProperty;
    for (Value::ObjectType::const_iterator it = objectValue.begin();
        it != objectValue.end(); ++it)
    {
        const std::string& propName = it->first;
        if (propName == Keys::ITEMID)
        {
            item.idStr_ = asString(it->second);
        }
        else
        {
            if (recommendSchema.getItemProperty(propName, recommendProperty))
            {
                item.propValueMap_[propName].assign(
                    asString(it->second),
                    kEncoding
                );
            }
            else
            {
                response().addError("Unknown item property " + propName + " in request[resource].");
                return false;
            }
        }
    }

    return true;
}

/**
 * @brief Action @b add_user. Add a user profile.
 *
 * @section request
 *
 * - @b collection* (@c String): Add user in this collection.
 * - @b resource* (@c Object): A user resource. Property key name is used as
 *   key. The corresponding value is the content of the property. Property @b
 *   USERID is required, which is a unique user identifier specified by
 *   client.
 *
 * @section response
 *
 * - @b head* (@c Object): Property @b success gives the result, true or false.
 *
 * @section example
 *
 * Request
 * @code
 * {
 *   "resource" => {
 *     "USERID": "user_001",
 *     "gender": "male",
 *     "age": "20",
 *     "area": "Shanghai"
 *   }
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
void RecommendController::add_user()
{
    IZENELIB_DRIVER_BEFORE_HOOK(requireProperty(Keys::USERID));

    User user;
    IZENELIB_DRIVER_BEFORE_HOOK(value2User(request()[Keys::resource], user));

    RecommendTaskService* service = collectionHandler_->recommendTaskService_;	
    if (!service->addUser(user))
    {
        response().addError("Failed to add user to given collection.");
    }
}

/**
 * @brief Action @b update_user. Update a user profile.
 *
 * @section request
 *
 * - @b collection* (@c String): Update user in this collection.
 * - @b resource* (@c Object): A user resource. Property key name is used as
 *   key. The corresponding value is the content of the property. Property @b
 *   USERID is required, which is a unique user identifier specified by
 *   client.
 *
 * @section response
 *
 * - @b head* (@c Object): Property @b success gives the result, true or false.
 *
 * @section example
 *
 * Request
 * @code
 * {
 *   "resource" => {
 *     "USERID": "user_001",
 *     "gender": "male",
 *     "age": "20",
 *     "area": "Shanghai"
 *   }
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
void RecommendController::update_user()
{
    IZENELIB_DRIVER_BEFORE_HOOK(requireProperty(Keys::USERID));

    User user;
    IZENELIB_DRIVER_BEFORE_HOOK(value2User(request()[Keys::resource], user));

    RecommendTaskService* service = collectionHandler_->recommendTaskService_;	
    if (!service->updateUser(user))
    {
        response().addError("Failed to update user to given collection.");
    }
}

/**
 * @brief Action @b remove_user. Remove user by user id.
 *
 * @section request
 *
 * - @b collection* (@c String): Search in this collection.
 * - @b resource* (@c Object): Only field @b USERID is used to remove the user.
 *
 * @section response
 *
 * - @b head* (@c Object): Property @b success gives the result, true or false.
 *
 * @section example
 *
 * Request
 * @code
 * {
 *   "resource" => {
 *     "USERID": "user_001"
 *   }
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
void RecommendController::remove_user()
{
    IZENELIB_DRIVER_BEFORE_HOOK(requireProperty(Keys::USERID));

    std::string userIdStr = asString(request()[Keys::resource][Keys::USERID]);

    RecommendTaskService* service = collectionHandler_->recommendTaskService_;	
    if (!service->removeUser(userIdStr))
    {
        response().addError("Failed to remove user from given collection.");
    }
}

/**
 * @brief Action @b get_user. Get user from SF1 by user id.
 *
 * @section request
 *
 * - @b collection* (@c String): Search in this collection.
 * - @b resource* (@c Object): Only field @b USERID is used to get the user.
 *
 * @section response
 *
 * - @b head* (@c Object): Property @b success gives the result, true or false.
 * - @b resource* (@c Object): A user resource. Property key name is used as
 *   key. The corresponding value is the content of the property.
 *
 * @section example
 *
 * Request
 * @code
 * {
 *   "resource" => {
 *     "USERID": "user_001"
 *   }
 * }
 * @endcode
 *
 * Response
 * @code
 * {
 *   "header": {"success": true},
 *   "resource" => {
 *     "USERID": "user_001",
 *     "gender": "male",
 *     "age": "20",
 *     "area": "Shanghai"
 *   }
 * }
 * @endcode
 */
void RecommendController::get_user()
{
    IZENELIB_DRIVER_BEFORE_HOOK(requireProperty(Keys::USERID));

    User user;
    std::string userIdStr = asString(request()[Keys::resource][Keys::USERID]);

    RecommendSearchService* service = collectionHandler_->recommendSearchService_;
    if (service->getUser(userIdStr, user))
    {
        Value& resource = response()[Keys::resource];
        resource[Keys::USERID] = user.idStr_;

        std::string convertBuffer;
        for (User::PropValueMap::const_iterator it = user.propValueMap_.begin();
            it != user.propValueMap_.end(); ++it)
        {
            it->second.convertString(convertBuffer, kEncoding);
            resource[it->first] = convertBuffer;
        }
    }
    else
    {
        response().addError("Failed to get user from given collection.");
    }
}

/**
 * @brief Action @b add_item. Add a product item.
 *
 * @section request
 *
 * - @b collection* (@c String): Add item in this collection.
 * - @b resource* (@c Object): An item resource. Property key name is used as
 *   key. The corresponding value is the content of the property. Property @b
 *   ITEMID is required, which is a unique item identifier specified by
 *   client.
 *
 * @section response
 *
 * - @b head* (@c Object): Property @b success gives the result, true or false.
 *
 * @section example
 *
 * Request
 * @code
 * {
 *   "resource" => {
 *     "ITEMID": "item_001",
 *     "name": "iphone",
 *     "link": "www.shop.com/product/item_001",
 *     "price": "5000"
 *     "category": "digital"
 *   }
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
void RecommendController::add_item()
{
    IZENELIB_DRIVER_BEFORE_HOOK(requireProperty(Keys::ITEMID));

    Item item;
    IZENELIB_DRIVER_BEFORE_HOOK(value2Item(request()[Keys::resource], item));

    RecommendTaskService* service = collectionHandler_->recommendTaskService_;	
    if (!service->addItem(item))
    {
        response().addError("Failed to add item to given collection.");
    }
}

/**
 * @brief Action @b update_item. Update a product item.
 *
 * @section request
 *
 * - @b collection* (@c String): Update item in this collection.
 * - @b resource* (@c Object): An item resource. Property key name is used as
 *   key. The corresponding value is the content of the property. Property @b
 *   ITEMID is required, which is a unique item identifier specified by
 *   client.
 *
 * @section response
 *
 * - @b head* (@c Object): Property @b success gives the result, true or false.
 *
 * @section example
 *
 * Request
 * @code
 * {
 *   "resource" => {
 *     "ITEMID": "item_001",
 *     "name": "iphone",
 *     "link": "www.shop.com/product/item_001",
 *     "price": "5000"
 *     "category": "digital"
 *   }
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
void RecommendController::update_item()
{
    IZENELIB_DRIVER_BEFORE_HOOK(requireProperty(Keys::ITEMID));

    Item item;
    IZENELIB_DRIVER_BEFORE_HOOK(value2Item(request()[Keys::resource], item));

    RecommendTaskService* service = collectionHandler_->recommendTaskService_;	
    if (!service->updateItem(item))
    {
        response().addError("Failed to update item to given collection.");
    }
}

/**
 * @brief Action @b remove_item. Remove a product by item id.
 *
 * @section request
 *
 * - @b collection* (@c String): Search in this collection.
 * - @b resource* (@c Object): Only field @b ITEMID is used to remove the item.
 *
 * @section response
 *
 * - @b head* (@c Object): Property @b success gives the result, true or false.
 *
 * @section example
 *
 * Request
 * @code
 * {
 *   "resource" => {
 *     "ITEMID": "item_001"
 *   }
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
void RecommendController::remove_item()
{
    IZENELIB_DRIVER_BEFORE_HOOK(requireProperty(Keys::ITEMID));

    std::string itemIdStr = asString(request()[Keys::resource][Keys::ITEMID]);

    RecommendTaskService* service = collectionHandler_->recommendTaskService_;	
    if (!service->removeItem(itemIdStr))
    {
        response().addError("Failed to remove item from given collection.");
    }
}

/**
 * @brief Action @b get_item. Get item from SF1 by item id.
 *
 * @section request
 *
 * - @b collection* (@c String): Search in this collection.
 * - @b resource* (@c Object): Only field @b ITEMID is used to get the item.
 *
 * @section response
 *
 * - @b head* (@c Object): Property @b success gives the result, true or false.
 * - @b resource* (@c Object): An item resource. Property key name is used as
 *   key. The corresponding value is the content of the property.
 *
 * @section example
 *
 * Request
 * @code
 * {
 *   "resource" => {
 *     "ITEMID": "item_001"
 *   }
 * }
 * @endcode
 *
 * Response
 * @code
 * {
 *   "header": {"success": true},
 *   "resource" => {
 *     "ITEMID": "item_001",
 *     "name": "iphone",
 *     "link": "www.shop.com/product/item_001",
 *     "price": "5000"
 *     "category": "digital"
 *   }
 * }
 * @endcode
 */
void RecommendController::get_item()
{
    IZENELIB_DRIVER_BEFORE_HOOK(requireProperty(Keys::ITEMID));

    Item item;
    std::string itemIdStr = asString(request()[Keys::resource][Keys::ITEMID]);

    RecommendSearchService* service = collectionHandler_->recommendSearchService_;
    if (service->getItem(itemIdStr, item))
    {
        Value& resource = response()[Keys::resource];
        resource[Keys::ITEMID] = item.idStr_;

        std::string convertBuffer;
        for (Item::PropValueMap::const_iterator it = item.propValueMap_.begin();
            it != item.propValueMap_.end(); ++it)
        {
            it->second.convertString(convertBuffer, kEncoding);
            resource[it->first] = convertBuffer;
        }
    }
    else
    {
        response().addError("Failed to get item from given collection.");
    }
}

} // namespace sf1r
