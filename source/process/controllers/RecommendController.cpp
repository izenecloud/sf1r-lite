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

/**
 * @brief Action @b visit_item. Add a visit item event.
 *
 * @section request
 *
 * - @b collection* (@c String): Add visit item event in this collection.
 * - @b resource* (@c Object): A resource for a visit event, that is, user
 *   @b USERID visited item @b ITEMID.
 *   - @b USERID* (@c String): a unique user identifier.
 *   - @b ITEMID* (@c String): a unique item identifier.
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
void RecommendController::visit_item()
{
    IZENELIB_DRIVER_BEFORE_HOOK(requireProperty(Keys::USERID));
    IZENELIB_DRIVER_BEFORE_HOOK(requireProperty(Keys::ITEMID));

    std::string userIdStr = asString(request()[Keys::resource][Keys::USERID]);
    std::string itemIdStr = asString(request()[Keys::resource][Keys::ITEMID]);

    RecommendTaskService* service = collectionHandler_->recommendTaskService_;	
    if (!service->visitItem(userIdStr, itemIdStr))
    {
        response().addError("Failed to add visit to given collection.");
    }
}

/**
 * @brief Action @b purchase_item. Add a purchase item event.
 *
 * @section request
 *
 * - @b collection* (@c String): Add purchase item event in this collection.
 * - @b resource* (@c Object): A resource for a purchase event, that is, user
 *   @b USERID purchased @b items.
 *   - @b USERID* (@c String): a unique user identifier.
 *   - @b items* (@c Array): each is an item purchased.
 *     - @b ITEMID* (@c String): a unique item identifier.
 *     - @b price (@c Double): the price of each item.
 *     - @b quantity (@c Uint): the number of items purchased.
 *   - @b order_id (@c String): the order id.
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
 *     "items": [
 *       {"ITEMID": "item_001", "price": 100, "quantity": 1},
 *       {"ITEMID": "item_002", "price": 200, "quantity": 2},
 *       {"ITEMID": "item_003", "price": 300, "quantity": 3}
 *     ],
 *     "order_id": "order_001"
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
void RecommendController::purchase_item()
{
    IZENELIB_DRIVER_BEFORE_HOOK(requireProperty(Keys::USERID));

    std::string userIdStr = asString(request()[Keys::resource][Keys::USERID]);
    std::string orderIdStr = asString(request()[Keys::resource][Keys::order_id]);

    const izenelib::driver::Value& itemsValue = request()[Keys::resource][Keys::items];
    if (nullValue(itemsValue) || itemsValue.type() != izenelib::driver::Value::kArrayType)
    {
        response().addError("Require an array of items in request[resource][items].");
        return;
    }

    RecommendTaskService::OrderItemVec orderItemVec;
    for (std::size_t i = 0; i < itemsValue.size(); ++i)
    {
        const Value& itemValue = itemsValue(i);

        std::string itemIdStr = asString(itemValue[Keys::ITEMID]);
        if (itemIdStr.empty())
        {
            response().addError("Require property " + Keys::ITEMID + " for each item in request[resource][items].");
            return;
        }

        int quantity = asUint(itemValue[Keys::quantity]);
        double price = asDouble(itemValue[Keys::price]);
        orderItemVec.push_back(RecommendTaskService::OrderItem(itemIdStr, quantity, price));
    }

    RecommendTaskService* service = collectionHandler_->recommendTaskService_;	
    if (!service->purchaseItem(userIdStr, orderItemVec, orderIdStr))
    {
        response().addError("Failed to add purchase to given collection.");
    }
}

} // namespace sf1r
