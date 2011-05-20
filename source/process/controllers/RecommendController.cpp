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
#include <recommend-manager/RecTypes.h>

#include <common/Keys.h>

namespace
{
/// @brief default count of recommended items in each request.
const std::size_t kDefaultRecommendCount = 10;

/// @brief default min frequency of each bundle in @p top_item_bundle().
const std::size_t kDefaultMinFreq = 1;

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

bool RecommendController::value2ItemIdVec(const std::string& propName, std::vector<std::string>& itemIdVec)
{
    const izenelib::driver::Value& itemVecValue = request()[Keys::resource][propName];

    if (nullValue(itemVecValue))
    {
        return true;
    }

    if (itemVecValue.type() != izenelib::driver::Value::kArrayType)
    {
        response().addError("Require an array of items in request[resource][" + propName + "].");
        return false;
    }

    for (std::size_t i = 0; i < itemVecValue.size(); ++i)
    {
        const Value& itemValue = itemVecValue(i);

        std::string itemIdStr;
        if (itemValue.type() == Value::kObjectType)
        {
            itemIdStr = asString(itemValue[Keys::ITEMID]);
        }
        else
        {
            itemIdStr = asString(itemValue);
        }

        if (itemIdStr.empty())
        {
            response().addError("Require property " + Keys::ITEMID + " for each item in request[resource][" + propName + "].");
            return false;
        }

        itemIdVec.push_back(itemIdStr);
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
 * - @b header (@c Object): Property @b success gives the result, true or false.
 *
 * @section example
 *
 * Request
 * @code
 * {
 *   "resource": {
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
 * - @b header (@c Object): Property @b success gives the result, true or false.
 *
 * @section example
 *
 * Request
 * @code
 * {
 *   "resource": {
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
 * - @b header (@c Object): Property @b success gives the result, true or false.
 *
 * @section example
 *
 * Request
 * @code
 * {
 *   "resource": {
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
 * - @b header (@c Object): Property @b success gives the result, true or false.
 * - @b resource (@c Object): A user resource. Property key name is used as
 *   key. The corresponding value is the content of the property.
 *
 * @section example
 *
 * Request
 * @code
 * {
 *   "resource": {
 *     "USERID": "user_001"
 *   }
 * }
 * @endcode
 *
 * Response
 * @code
 * {
 *   "header": {"success": true},
 *   "resource": {
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
 * - @b header (@c Object): Property @b success gives the result, true or false.
 *
 * @section example
 *
 * Request
 * @code
 * {
 *   "resource": {
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
 * - @b header (@c Object): Property @b success gives the result, true or false.
 *
 * @section example
 *
 * Request
 * @code
 * {
 *   "resource": {
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
 * - @b header (@c Object): Property @b success gives the result, true or false.
 *
 * @section example
 *
 * Request
 * @code
 * {
 *   "resource": {
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
 * - @b header (@c Object): Property @b success gives the result, true or false.
 * - @b resource (@c Object): An item resource. Property key name is used as
 *   key. The corresponding value is the content of the property.
 *
 * @section example
 *
 * Request
 * @code
 * {
 *   "resource": {
 *     "ITEMID": "item_001"
 *   }
 * }
 * @endcode
 *
 * Response
 * @code
 * {
 *   "header": {"success": true},
 *   "resource": {
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
 * - @b header (@c Object): Property @b success gives the result, true or false.
 *
 * @section example
 *
 * Request
 * @code
 * {
 *   "resource": {
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
 * - @b header (@c Object): Property @b success gives the result, true or false.
 *
 * @section example
 *
 * Request
 * @code
 * {
 *   "resource": {
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
    if (!service->purchaseItem(userIdStr, orderIdStr, orderItemVec))
    {
        response().addError("Failed to add purchase to given collection.");
    }
}

/**
 * @brief Action @b do_recommend. Get recommendation result.
 *
 * @section request
 *
 * - @b collection* (@c String): Get recommendation result in this collection.
 * - @b resource* (@c Object): A resource of the request for recommendation result.
 *   - @b rec_type_id* (@c Uint): recommendation type, now 6 types are supported, each with the id below:
 *     - @b 0 (<b>Frequently Bought Together</b>): get the items frequently bought together with @b input_items in one order.
 *     - @b 1 (<b>Bought Also Bought</b>): get the items also bought by the users who have bought @b input_items.
 *     - @b 2 (<b>Viewed Also View</b>): get the items also viewed by the users who have viewed @b input_items,
 *       in current version, it supports recommending items based on only one input item,
 *       that is, only <b> input_items[0]</b> is used as input , and the rest items in @b input_items are ignored.
 *     - @b 3 (<b>Based on Purchase History</b>): get the recommendation items based on the purchase history of user @b USERID.
 *       The purchase history is the items added in purchase_item() with the same @b USERID.
 *     - @b 4 (<b>Based on Browse History</b>): get the recommendation items based on the browse history of user @b USERID.
 *       @b input_items could be specified as browse history. If @b USERID is specified, both @b input_items and the items
 *       added in visit_item() with the same @b USERID would be excluded in recommendation result. Otherwise, if @b USERID
 *       is not specified for anonymous users, the recommendation would be based on @b input_items instead.
 *     - @b 5 (<b>Based on Shopping Cart</b>): get the recommendation items based on the shopping cart of user @b USERID,
 *       @b input_items could be specified as the items in shopping cart. If @b USERID is specified, @b input_items would be
 *       excluded in recommendation result. Otherwise, if @b USERID is not specified for anonymous users, the recommendation
 *       would be based on @b input_items instead.
 *   - @b max_count (@c Uint = 10): max item number allowed in recommendation result.
 *   - @b USERID (@c String): a unique user identifier.
 *   - @b input_items (@c Array): the input items for recommendation.
 *     - @b ITEMID* (@c String): a unique item identifier.
 *   - @b include_items (@c Array): the items must be included in recommendation result.
 *     - @b ITEMID* (@c String): a unique item identifier.
 *   - @b exclude_items (@c Array): the items must be excluded in recommendation result.
 *     - @b ITEMID* (@c String): a unique item identifier.
 *
 * @section response
 *
 * - @b header (@c Object): Property @b success gives the result, true or false.
 * - @b resources (@c Array): each is an item in recommendation result.
 *   - @b ITEMID (@c String): a unique item identifier.
 *   - @b weight (@c Double): the recommendation weight, if this value is available, the items would be sorted by this value decreasingly.
 *   - each item properties in add_item() would also be included here. Property key name is used as key.
 *     The corresponding value is the content of the property.
 *
 * @section example
 *
 * Request
 * @code
 * {
 *   "resource": {
 *     "rec_type_id": 3,
 *     "max_count": 20,
 *     "USERID": "user_001"
 *   }
 * }
 * @endcode
 *
 * Response
 * @code
 * {
 *   "header": {"success": true},
 *   "resources": [
 *     {"ITEMID": "item_001", "weight": 0.9, "name": "iphone", "link": "www.shop.com/product/item_001", "price": "5000", "category": "digital"},
 *     {"ITEMID": "item_002", "weight": 0.8, "name": "ipad", "link": "www.shop.com/product/item_002", "price": "6000", "category": "digital"},
 *     {"ITEMID": "item_003", "weight": 0.7, "name": "imac", "link": "www.shop.com/product/item_003", "price": "20000", "category": "digital"}
 *     ...
 *   ]
 * }
 * @endcode
 */
void RecommendController::do_recommend()
{
    IZENELIB_DRIVER_BEFORE_HOOK(requireProperty(Keys::rec_type_id));

    int maxCount = kDefaultRecommendCount;
    const izenelib::driver::Value& maxCountValue = request()[Keys::resource][Keys::max_count];
    if (!nullValue(maxCountValue))
    {
        maxCount = asUint(maxCountValue);
    }
    if (maxCount <= 0)
    {
        response().addError("Require a positive value in request[resource][max_count].");
        return;
    }

    std::vector<std::string> inputItemVec;
    std::vector<std::string> includeItemVec;
    std::vector<std::string> excludeItemVec;
    if (!value2ItemIdVec(Keys::input_items, inputItemVec)
        || !value2ItemIdVec(Keys::include_items, includeItemVec)
        || !value2ItemIdVec(Keys::exclude_items, excludeItemVec))
    {
        return;
    }

    std::string userIdStr = asString(request()[Keys::resource][Keys::USERID]);
    int recTypeId = asUint(request()[Keys::resource][Keys::rec_type_id]);
    switch (recTypeId)
    {
        case FREQUENT_BUY_TOGETHER:
        case BUY_ALSO_BUY:
        case VIEW_ALSO_VIEW:
        {
            if (inputItemVec.empty())
            {
                response().addError("This recommendation type requires items sepcified in request[resource][input_items].");
                return;
            }
            break;
        }

        case BASED_ON_PURCHASE_HISTORY:
        {
            if (userIdStr.empty())
            {
                response().addError("This recommendation type requires user id sepcified in request[resource][USERID].");
                return;
            }
            break;
        }

        case BASED_ON_BROWSE_HISTORY:
        case BASED_ON_SHOP_CART:
        {
            if (userIdStr.empty() && inputItemVec.empty())
            {
                response().addError("This recommendation type requires either USERID or input_items sepcified in request[resource].");
                return;
            }
            break;
        }

        default:
        {
            response().addError("Unknown recommendation type in request[resource][rec_type_id].");
            return;
        }
    }

    std::vector<Item> recItemVec;
    std::vector<double> recWeightVec;
    RecommendSearchService* service = collectionHandler_->recommendSearchService_;
    if (service->recommend(static_cast<RecommendType>(recTypeId), maxCount, userIdStr,
                           inputItemVec, includeItemVec, excludeItemVec,
                           recItemVec, recWeightVec))
    {
        if (recItemVec.size() != recWeightVec.size())
        {
            response().addError("Failed to get recommendation result from given collection (unequal size of item and weight array).");
            return;
        }

        Value& resources = response()[Keys::resources];
        std::string convertBuffer;
        for (std::size_t i = 0; i < recItemVec.size(); ++i)
        {
            Value& itemValue = resources();
            itemValue[Keys::weight] = recWeightVec[i];

            Item& item = recItemVec[i];
            itemValue[Keys::ITEMID] = item.idStr_;
            for (Item::PropValueMap::const_iterator it = item.propValueMap_.begin();
                it != item.propValueMap_.end(); ++it)
            {
                it->second.convertString(convertBuffer, kEncoding);
                itemValue[it->first] = convertBuffer;
            }
        }
    }
    else
    {
        response().addError("Failed to get recommendation result from given collection.");
    }
}

/**
 * @brief Action @b top_item_bundle. Get the most frequent item bundles, with each bundle containing the items frequently bought together in one order.
 *
 * @section request
 *
 * - @b collection* (@c String): Get recommendation result in this collection.
 * - @b resource* (@c Object): A resource of the request for recommendation result.
 *   - @b max_count (@c Uint = 10): at most how many bundles allowed in result.
 *   - @b min_freq (@c Uint = 1): the min frequency for each bundle in result.
 *
 * @section response
 *
 * - @b header (@c Object): Property @b success gives the result, true or false.
 * - @b resources (@c Array): each is a bundle, sorted by @b freq decreasingly.
 *   - @b freq (@c Uint): the frequency of the bundle, that is, how many times this bundle is contained in all orders.
 *   - @b items (@c Array): each is an item in the bundle.
 *     - @b ITEMID (@c String): a unique item identifier.
 *     - each item properties in add_item() would also be included here. Property key name is used as key.
 *       The corresponding value is the content of the property.
 *
 * @section example
 *
 * Request
 * @code
 * {
 *   "resource": {
 *     "max_count": 20,
 *     "min_freq": 10
 *   }
 * }
 * @endcode
 *
 * Response
 * @code
 * {
 *   "header": {"success": true},
 *   "resources": [
 *     { "freq": 90,
 *       "items": [
 *         {"ITEMID": "item_001", "name": "iphone", "link": "www.shop.com/product/item_001", "price": "5000", "category": "digital"},
 *         {"ITEMID": "item_002", "name": "ipad", "link": "www.shop.com/product/item_002", "price": "6000", "category": "digital"}
 *       ]
 *     },
 *     { "freq": 85,
 *       "items": [
 *         {"ITEMID": "item_002", "name": "ipad", "link": "www.shop.com/product/item_002", "price": "6000", "category": "digital"},
 *         {"ITEMID": "item_003", "name": "imac", "link": "www.shop.com/product/item_003", "price": "30000", "category": "digital"}
 *       ]
 *     }
 *     ...
 *   ]
 * }
 * @endcode
 */
void RecommendController::top_item_bundle()
{
    int maxCount = kDefaultRecommendCount;
    const izenelib::driver::Value& maxCountValue = request()[Keys::resource][Keys::max_count];
    if (!nullValue(maxCountValue))
    {
        maxCount = asUint(maxCountValue);
    }
    if (maxCount <= 0)
    {
        response().addError("Require a positive value in request[resource][max_count].");
        return;
    }

    int minFreq = kDefaultMinFreq;
    const izenelib::driver::Value& minFreqValue = request()[Keys::resource][Keys::min_freq];
    if (!nullValue(minFreqValue))
    {
        minFreq = asUint(minFreqValue);
    }

    std::vector<vector<Item> > bundleVec;
    std::vector<int> freqVec;
    RecommendSearchService* service = collectionHandler_->recommendSearchService_;
    if (service->topItemBundle(maxCount, minFreq,
                               bundleVec, freqVec))
    {
        if (bundleVec.size() != freqVec.size())
        {
            response().addError("Failed to get top item bundle from given collection (unequal size of bundle and freq array).");
            return;
        }

        Value& resources = response()[Keys::resources];
        std::string convertBuffer;
        for (std::size_t i = 0; i < bundleVec.size(); ++i)
        {
            Value& bundleValue = resources();
            bundleValue[Keys::freq] = freqVec[i];

            Value& itemsValue = bundleValue[Keys::items];
            const std::vector<Item>& itemVec = bundleVec[i];
            for (std::size_t j = 0; j < itemVec.size(); ++j)
            {
                Value& itemValue = itemsValue();
                const Item& item = itemVec[j];
                itemValue[Keys::ITEMID] = item.idStr_;
                for (Item::PropValueMap::const_iterator it = item.propValueMap_.begin();
                    it != item.propValueMap_.end(); ++it)
                {
                    it->second.convertString(convertBuffer, kEncoding);
                    itemValue[it->first] = convertBuffer;
                }
            }
        }
    }
    else
    {
        response().addError("Failed to get top item bundle from given collection.");
    }
}

} // namespace sf1r
