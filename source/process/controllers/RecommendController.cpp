/**
 * @file process/controllers/RecommendController.cpp
 * @author Jun Jiang
 * @date Created <2011-04-21>
 */
#include "RecommendController.h"
#include "CollectionHandler.h"

#include <bundles/recommend/RecommendTaskService.h>
#include <bundles/recommend/RecommendSearchService.h>
#include <recommend-manager/User.h>
#include <recommend-manager/RecommendItem.h>
#include <recommend-manager/ItemCondition.h>
#include <recommend-manager/RecTypes.h>
#include <recommend-manager/RecommendParam.h>
#include <recommend-manager/TIBParam.h>
#include <recommend-manager/ItemBundle.h>
#include <recommend-manager/RateParam.h>

#include <common/IndexBundleSchemaHelpers.h>
#include <common/Keys.h>
#include <common/Utilities.h> // Utilities::toUpper()

namespace
{
/// @brief default count of recommended items in each request.
const std::size_t kDefaultRecommendCount = 10;

/// @brief default min frequency of each bundle in @p top_item_bundle().
const std::size_t kDefaultMinFreq = 1;

const izenelib::util::UString::EncodingType ENCODING_TYPE = izenelib::util::UString::UTF_8;

void renderPropertyValue(
    const sf1r::PropertyValue& propValue,
    izenelib::driver::Value& renderValue
)
{
    const izenelib::util::UString& ustr = propValue.get<izenelib::util::UString>();
    std::string utf8;
    ustr.convertString(utf8, ENCODING_TYPE);

    renderValue = utf8;
}

const std::string DOCID("DOCID");
const std::string ITEMID("ITEMID");

const std::string& docIdToItemId(const std::string& propName)
{
    if (propName == DOCID)
        return ITEMID;

    return propName;
}

void itemIdToDocId(std::string& propName)
{
    if (propName == ITEMID)
        propName = DOCID;
}

}

namespace sf1r
{

using namespace izenelib::driver;
using driver::Keys;

RecommendController::RecommendController()
{
    recTypeMap_["FBT"] = FREQUENT_BUY_TOGETHER;
    recTypeMap_["BAB"] = BUY_ALSO_BUY;
    recTypeMap_["VAV"] = VIEW_ALSO_VIEW;
    recTypeMap_["BOE"] = BASED_ON_EVENT;
    recTypeMap_["BOB"] = BASED_ON_BROWSE_HISTORY;
    recTypeMap_["BOS"] = BASED_ON_SHOP_CART;
}

bool RecommendController::requireProperty(
    const std::string& propName,
    std::string& propValue
)
{
    const Value& resourceValue = request()[Keys::resource];

    if (! resourceValue.hasKey(propName))
    {
        response().addError("Require property \"" + propName + "\" in request[resource].");
        return false;
    }

    propValue = asString(resourceValue[propName]);
    if (propValue.empty())
    {
        response().addError("Require non-empty value for property \"" + propName + "\" in request[resource].");
        return false;
    }

    return true;
}

bool RecommendController::requireService(::izenelib::osgi::IService* service)
{
    if (! service)
    {
        response().addError("Request failed, no recommend service found.");
        return false;
    }

    return true;
}

bool RecommendController::value2User(User& user)
{
    if (! requireProperty(Keys::USERID, user.idStr_))
        return false;

    const Value& resourceValue = request()[Keys::resource];
    const Value::ObjectType& objectValue = resourceValue.getObject();
    const RecommendSchema& recommendSchema = collectionHandler_->recommendSchema_;
    RecommendProperty recommendProperty;

    for (Value::ObjectType::const_iterator it = objectValue.begin();
        it != objectValue.end(); ++it)
    {
        const std::string& propName = it->first;
        if (propName != Keys::USERID)
        {
            if (recommendSchema.getUserProperty(propName, recommendProperty))
            {
                user.propValueMap_[propName].assign(asString(it->second),
                                                    ENCODING_TYPE);
            }
            else
            {
                response().addError("Unknown user property \"" + propName + "\" in request[resource].");
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
            response().addError("Require property \"" + Keys::ITEMID + "\" for each item in request[resource][" + propName + "].");
            return false;
        }

        itemIdVec.push_back(itemIdStr);
    }

    return true;
}

bool RecommendController::value2ItemCondition(ItemCondition& itemCondition)
{
    const izenelib::driver::Value& condValue = request()[Keys::resource][Keys::condition];

    if (nullValue(condValue))
    {
        return true;
    }

    if (condValue.type() != izenelib::driver::Value::kObjectType)
    {
        response().addError("Require a map in request[resource][condition].");
        return false;
    }

    std::string propName = asString(condValue[Keys::property]);
    if (propName.empty())
    {
        response().addError("Require \"" + Keys::property + "\" in request[resource][condition].");
        return false;
    }
    itemIdToDocId(propName);

    PropertyConfig propType;
    if (! getPropertyConfig(collectionHandler_->indexSchema_, propName, propType))
    {
        response().addError("Unknown item property \"" + propName + "\" in request[resource][condition].");
        return false;
    }
    itemCondition.propName_ = propName;

    const izenelib::driver::Value& arrayValue = condValue[Keys::value];
    if (arrayValue.type() != izenelib::driver::Value::kArrayType)
    {
        response().addError("Require an array of property values in request[resource][condition][value].");
        return false;
    }

    for (std::size_t i = 0; i < arrayValue.size(); ++i)
    {
        std::string valueStr = asString(arrayValue(i));

        if (valueStr.empty())
        {
            response().addError("Invalid property value in request[resource][condition][value].");
            return false;
        }

        itemCondition.propValueSet_.insert(izenelib::util::UString(valueStr, ENCODING_TYPE));
    }

    if (itemCondition.propValueSet_.empty())
    {
        response().addError("No property values in request[resource][condition][value].");
        return false;
    }

    return true;
}

/**
 * @brief Action @b add_user. Add a user profile.
 *
 * @section request
 *
 * - @b collection* (@c String): Add user in this collection.
 * - @b resource* (@c Object): A user resource.@n
 *   Property key name is used as key. The corresponding value is the content of the property.@n
 *   Property @b USERID is required, which is a unique user identifier specified by client.
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
    RecommendTaskService* service = collectionHandler_->recommendTaskService_;
    User user;

    IZENELIB_DRIVER_BEFORE_HOOK(requireService(service)
                                && value2User(user));

    if (! service->addUser(user))
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
 * - @b resource* (@c Object): A user resource.@n
 *   Property key name is used as key. The corresponding value is the content of the property.@n
 *   Property @b USERID is required, which is a unique user identifier specified by client.
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
    RecommendTaskService* service = collectionHandler_->recommendTaskService_;
    User user;

    IZENELIB_DRIVER_BEFORE_HOOK(requireService(service)
                                && value2User(user));

    if (! service->updateUser(user))
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
    RecommendTaskService* service = collectionHandler_->recommendTaskService_;
    std::string userIdStr;
    IZENELIB_DRIVER_BEFORE_HOOK(requireService(service)
                                && requireProperty(Keys::USERID, userIdStr));

    if (! service->removeUser(userIdStr))
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
 * - @b resource (@c Object): A user resource.@n
 *   Property key name is used as key. The corresponding value is the content of the property.
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
    RecommendSearchService* service = collectionHandler_->recommendSearchService_;
    std::string userIdStr;
    IZENELIB_DRIVER_BEFORE_HOOK(requireService(service)
                                && requireProperty(Keys::USERID, userIdStr));

    User user;
    if (service->getUser(userIdStr, user))
    {
        Value& resource = response()[Keys::resource];
        resource[Keys::USERID] = user.idStr_;

        std::string convertBuffer;
        for (User::PropValueMap::const_iterator it = user.propValueMap_.begin();
            it != user.propValueMap_.end(); ++it)
        {
            it->second.convertString(convertBuffer, ENCODING_TYPE);
            resource[it->first] = convertBuffer;
        }
    }
    else
    {
        response().addError("Failed to get user from given collection.");
    }
}

/**
 * @brief Action @b visit_item. Add a visit item event.
 *
 * @section request
 *
 * - @b collection* (@c String): Add visit item event in this collection.
 * - @b resource* (@c Object): A resource for a visit event, that is, user @b USERID visited item @b ITEMID.
 *   - @b session_id* (@c String): a session id.@n
 *     A session id is a unique identifier to identify the user's current interaction session.@n
 *     For each session between user logging in and logging out, the session id should be unique and not changed.@n
 *     In each session, the items visited by the user would be used to recommend items for the type @b VAV in @c do_recommend().
 *   - @b USERID* (@c String): a unique user identifier.
 *   - @b ITEMID* (@c String): a unique item identifier.
 *   - @b is_recommend_item (@c Bool = @c false): this is an important flag to give feedback on recommendation result.@n
 *     @c true for @b USERID visits @b ITEMID because it was recommended to that user before,@n
 *     otherwise, @c false is set.
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
 *     "session_id": "session_001",
 *     "USERID": "user_001",
 *     "ITEMID": "item_001",
 *     "is_recommend_item": true
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
    RecommendTaskService* service = collectionHandler_->recommendTaskService_;
    std::string sessionIdStr, userIdStr, itemIdStr;
    IZENELIB_DRIVER_BEFORE_HOOK(requireService(service)
                                && requireProperty(Keys::session_id, sessionIdStr)
                                && requireProperty(Keys::USERID, userIdStr)
                                && requireProperty(Keys::ITEMID, itemIdStr));

    izenelib::driver::Value& resourceValue = request()[Keys::resource];
    bool isRecItem = asBoolOr(resourceValue[Keys::is_recommend_item], false);

    if (! service->visitItem(sessionIdStr, userIdStr, itemIdStr, isRecItem))
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
 * - @b resource* (@c Object): A resource for a purchase event, that is, user @b USERID purchased @b items.
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
    RecommendTaskService* service = collectionHandler_->recommendTaskService_;
    std::string userIdStr;
    IZENELIB_DRIVER_BEFORE_HOOK(requireService(service)
                                && requireProperty(Keys::USERID, userIdStr));

    izenelib::driver::Value& resourceValue = request()[Keys::resource];
    std::string orderIdStr = asString(resourceValue[Keys::order_id]);

    const izenelib::driver::Value& itemsValue = resourceValue[Keys::items];
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
            response().addError("Require property \"" + Keys::ITEMID + "\" for each item in request[resource][items].");
            return;
        }

        int quantity = asUint(itemValue[Keys::quantity]);
        double price = asDouble(itemValue[Keys::price]);
        orderItemVec.push_back(RecommendTaskService::OrderItem(itemIdStr, quantity, price));
    }

    if (! service->purchaseItem(userIdStr, orderIdStr, orderItemVec))
    {
        response().addError("Failed to add purchase to given collection.");
    }
}

/**
 * @brief Action @b update_shopping_cart. Update the item in shopping cart.
 *
 * @section request
 *
 * - @b collection* (@c String): Add event on updating shopping cart in this collection.
 * - @b resource* (@c Object): A resource for a shopping cart event, that is, the shopping cart of user @b USERID contains @b items.@n
 *   Please note that whenever there is any update in user's shopping cart, you have to specify @b items as all the items in the shopping cart.@n
 *   When the shopping cart becomes empty, you have to add this event by calling @c update_shopping_cart() with an empty @b items.
 *   - @b USERID* (@c String): a unique user identifier.
 *   - @b items* (@c Array): each is an item in shopping cart.
 *     - @b ITEMID* (@c String): a unique item identifier.
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
 *       {"ITEMID": "item_001"},
 *       {"ITEMID": "item_002"},
 *       {"ITEMID": "item_003"}
 *     ]
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
void RecommendController::update_shopping_cart()
{
    RecommendTaskService* service = collectionHandler_->recommendTaskService_;
    std::string userIdStr;
    IZENELIB_DRIVER_BEFORE_HOOK(requireService(service)
                                && requireProperty(Keys::USERID, userIdStr));

    izenelib::driver::Value& resourceValue = request()[Keys::resource];
    const izenelib::driver::Value& itemsValue = resourceValue[Keys::items];

    if (nullValue(itemsValue) || itemsValue.type() != izenelib::driver::Value::kArrayType)
    {
        response().addError("Require an array of items in request[resource][items].");
        return;
    }

    RecommendTaskService::OrderItemVec cartItemVec;
    for (std::size_t i = 0; i < itemsValue.size(); ++i)
    {
        const Value& itemValue = itemsValue(i);

        std::string itemIdStr = asString(itemValue[Keys::ITEMID]);
        if (itemIdStr.empty())
        {
            response().addError("Require property \"" + Keys::ITEMID + "\" for each item in request[resource][items].");
            return;
        }

        cartItemVec.push_back(RecommendTaskService::OrderItem(itemIdStr));
    }

    if (! service->updateShoppingCart(userIdStr, cartItemVec))
    {
        response().addError("Failed to update shopping cart to given collection.");
    }
}

/**
 * @brief Action @b track_event. Track an event of user behavior.
 *
 * @section request
 *
 * - @b collection* (@c String): Track user behavior in this collection.
 * - @b resource* (@c Object): A resource for a user behavior.
 *   - @b is_add (@c Bool = @c true): @c true for add this event, @c false for remove this event.
 *   - @b event* (@c String): the event type, it could be one of below values:@n
 *     - @b wish_list: user @b USERID added @b ITEMID in his/her wish list.
 *     - @b own: user @b USERID already owns @b ITEMID.
 *     - @b like: user @b USERID likes @b ITEMID.
 *     - @b favorite: user @b USERID added @b ITEMID into his/her favorite items.@n
 *     The above events are configured in <tt> <RecommendBundle><Schema><Track> </tt> in collection config file (such as <tt>config/example.xml</tt>),@n
 *     you could modify or add more events in configuration.@n
 *     Below event types are supported by default, they are used to exclude items:
 *     - @b not_rec_result: exclude @b ITEMID from being one of the recommendation results for @b USERID.
 *     - @b not_rec_input: exclude @b ITEMID from being used in making recommendation for @b USERID.
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
 *     "event": "wish_list",
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
void RecommendController::track_event()
{
    RecommendTaskService* service = collectionHandler_->recommendTaskService_;
    std::string eventStr, userIdStr, itemIdStr;
    IZENELIB_DRIVER_BEFORE_HOOK(requireService(service)
                                && requireProperty(Keys::event, eventStr)
                                && requireProperty(Keys::USERID, userIdStr)
                                && requireProperty(Keys::ITEMID, itemIdStr));

    izenelib::driver::Value& resourceValue = request()[Keys::resource];
    bool isAdd = asBoolOr(resourceValue[Keys::is_add], true);

    if (! collectionHandler_->recommendSchema_.hasEvent(eventStr))
    {
        response().addError("Unknown event type \"" + eventStr + "\" in request[resource].");
        return;
    }

    if (! service->trackEvent(isAdd, eventStr, userIdStr, itemIdStr))
    {
        response().addError("Failed to track event in given collection.");
    }
}

/**
 * @brief Action @b rate_item. Add a rating item event.
 *
 * @section request
 *
 * - @b collection* (@c String): Track user behavior in this collection.
 * - @b resource* (@c Object): A resource for a user behavior.
 *   - @b is_add (@c Bool = @c true): @c true for add this rating, @c false for remove this rating.
 *   - @b USERID* (@c String): a unique user identifier.
 *   - @b ITEMID* (@c String): a unique item identifier.
 *   - @b star (@c Uint): the rating star, an integral value from 1 to 5, each with meaning below:@n
 *   This parameter is required when @b is_add is @c true.
 *     - @b 1: I hate it
 *     - @b 2: I don't like it
 *     - @b 3: It's OK
 *     - @b 4: I like it
 *     - @b 5: I love it
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
 *     "ITEMID": "item_001",
 *     "star": 5
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
void RecommendController::rate_item()
{
    RecommendTaskService* service = collectionHandler_->recommendTaskService_;
    IZENELIB_DRIVER_BEFORE_HOOK(requireService(service));

    RateParam param;
    if (! parseRateParam(param))
        return;

    if (! service->rateItem(param))
    {
        response().addError("Failed to rate item in given collection.");
    }
}

bool RecommendController::parseRateParam(RateParam& param)
{
    if (! (requireProperty(Keys::USERID, param.userIdStr)
           && requireProperty(Keys::ITEMID, param.itemIdStr)))
        return false;

    izenelib::driver::Value& resourceValue = request()[Keys::resource];

    param.userIdStr = asString(resourceValue[Keys::USERID]);
    param.itemIdStr = asString(resourceValue[Keys::ITEMID]);
    param.isAdd = asBoolOr(resourceValue[Keys::is_add], true);
    param.rate = asUintOr(resourceValue[Keys::star], 0);

    std::string errorMsg;
    if (! param.check(errorMsg))
    {
        response().addError(errorMsg);
        return false;
    }

    return true;
}

/**
 * @brief Action @b do_recommend. Get recommendation result.
 *
 * @section request
 *
 * - @b collection* (@c String): Get recommendation result in this collection.
 * - @b resource* (@c Object): A resource of the request for recommendation result.
 *   - @b rec_type* (@c String): Specify one of the 6 recommendation types, each with the acronym below:
 *     - @b FBT (<b>Frequently Bought Together</b>): get the items frequently bought together with @b input_items in one order.
 *     - @b BAB (<b>Bought Also Bought</b>): get the items also bought by the users who have bought @b input_items.
 *     - @b VAV (<b>Viewed Also View</b>): get the items also viewed by the users who have viewed @b input_items.@n
 *       In current version, it supports recommending items based on only one input item,@n
 *       that is, only <b> input_items[0]</b> is used as input, and the rest items in @b input_items are ignored.
 *     - @b BOE (<b>Based on Event</b>): get the recommendation items based on the user events by @b USERID,@n
 *       which user events are from @c purchase_item(), @c update_shopping_cart(), @c track_event() and @c rate_item().@n
 *       The parameter @b input_items is not used for this recommendation type.
 *     - @b BOB (<b>Based on Browse History</b>): get the recommendation items based on the browse history of user @b USERID.@n
 *       If @b input_items is specified, the @b input_items would be used as the user's browse history.@n
 *       Otherwise, you have to specify both @b USERID and @b session_id, then the items added in @c visit_item() with the same @b USERID and @b session_id would be used as browse history.@n
 *       In the recommendation results, the items added by @c purchase_item(), @c update_shopping_cart(), @c track_event() and @c rate_item() would be excluded.
 *     - @b BOS (<b>Based on Shopping Cart</b>): get the recommendation items based on the shopping cart of user @b USERID.@n
 *       If @b input_items is specified, the @b input_items would be used as the items in user's shopping cart.@n
 *       Otherwise, the items added in @c update_shopping_cart() with the same @b USERID would be used as shoppint cart.@n
 *       In the recommendation results, the items added by @c purchase_item(), @c update_shopping_cart(), @c track_event() and @c rate_item() would be excluded.
 *   - @b max_count (@c Uint = 10): max item number allowed in recommendation result.
 *   - @b USERID (@c String): a unique user identifier.@n
 *     This parameter is required for rec_type of @b BOE, and optional for @b BOB and @b BOS.
 *   - @b session_id (@c String): a session id.@n
 *     A session id is a unique identifier to identify the user's current interaction session.@n
 *     For each session between user logging in and logging out, the session id should be unique and not changed.@n
 *     In current version, this parameter is only used in @b BOB rec_type.
 *   - @b input_items (@c Array): the input items for recommendation.@n
 *     This parameter is required for rec_type of @b FBT, @b BAB, @b VAV, and optional for @b BOB and @b BOS.
 *     - @b ITEMID* (@c String): a unique item identifier.
 *   - @b include_items (@c Array): the items must be included in recommendation result.
 *     - @b ITEMID* (@c String): a unique item identifier.
 *   - @b exclude_items (@c Array): the items must be excluded in recommendation result.
 *     - @b ITEMID* (@c String): a unique item identifier.
 *   - @b condition (@c Object): specify the condition that recommendation results must meet.
 *     - @b property* (@c String): item property name, such as @b ITEMID, @b category, etc
 *     - @b value* (@c Array): the property values, each recommendation result must match one of the property value in this array.
 *
 * @section response
 *
 * - @b header (@c Object): Property @b success gives the result, true or false.
 * - @b resources (@c Array): each is an item in recommendation result.
 *   - @b ITEMID (@c String): a unique item identifier.
 *   - @b weight (@c Double): the recommendation weight, if this value is available, the items would be sorted by this value decreasingly.
 *   - @b reasons (@c Array): the reasons why this item is recommended. Each is an event which has major influence on recommendation.@n
 *     Please note that the @b reasons result would only be returned for rec_type of @b BOE, @b BOB, @b BOS.
 *     - @b event (@c String): the event type, it could be @b purchase, @b shopping_cart, @b browse, @b rate, or the event values in @c track_event().
 *     - @b ITEMID (@c String): a unique item identifier, which item appears in the above event.
 *     - @b value (@c String): if @b event is @b rate, the @b value would be returned as rating star, such as "5".
 *   - The item properties would also be returned here, such as "name", "link", etc.@n
 *     Property key name is used as key. The corresponding value is the content of that property.
 *
 * @section example
 *
 * Request
 * @code
 * {
 *   "resource": {
 *     "rec_type": "BOE",
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
 *     {"ITEMID": "item_001", "weight": 0.9, "reasons": [{"event": "purchase"}, {"ITEMID": "item_011"}], "name": "iphone", "link": "www.shop.com/product/item_001", "price": "5000", "category": "digital"},
 *     {"ITEMID": "item_002", "weight": 0.8, "reasons": [{"event": "shopping_cart"}, {"ITEMID": "item_012"}], "name": "ipad", "link": "www.shop.com/product/item_002", "price": "6000", "category": "digital"},
 *     {"ITEMID": "item_003", "weight": 0.7, "reasons": [{"event": "wish_list"}, {"ITEMID": "item_013"}], "name": "imac", "link": "www.shop.com/product/item_003", "price": "20000", "category": "digital"}
 *     ...
 *   ]
 * }
 * @endcode
 */
void RecommendController::do_recommend()
{
    RecommendSearchService* service = collectionHandler_->recommendSearchService_;
    IZENELIB_DRIVER_BEFORE_HOOK(requireService(service));

    RecommendParam recParam;
    if (! parseRecommendParam(recParam))
        return;

    std::vector<RecommendItem> recItemVec;
    if (service->recommend(recParam, recItemVec))
    {
        renderRecommendResult(recParam, recItemVec);
    }
    else
    {
        response().addError("Failed to get recommendation result from given collection.");
    }
}

bool RecommendController::parseRecommendParam(RecommendParam& param)
{
    std::string recTypeStr;
    if (! requireProperty(Keys::rec_type, recTypeStr))
        return false;

    izenelib::driver::Value& resourceValue = request()[Keys::resource];
    param.limit = asUintOr(resourceValue[Keys::max_count],
                           kDefaultRecommendCount);

    if (! (value2ItemIdVec(Keys::input_items, param.inputItems)
           && value2ItemIdVec(Keys::include_items, param.includeItems)
           && value2ItemIdVec(Keys::exclude_items, param.excludeItems)
           && value2ItemCondition(param.condition)))
    {
        return false;
    }

    param.userIdStr = asString(resourceValue[Keys::USERID]);
    param.sessionIdStr = asString(resourceValue[Keys::session_id]);

    std::map<std::string, int>::const_iterator mapIt = recTypeMap_.find(Utilities::toUpper(recTypeStr));
    if (mapIt == recTypeMap_.end())
    {
        response().addError("Unknown recommendation type \"" + recTypeStr + "\" in request[resource][rec_type].");
        return false;
    }
    param.type = static_cast<RecommendType>(mapIt->second);

    std::string errorMsg;
    if (! param.check(errorMsg))
    {
        response().addError(errorMsg);
        return false;
    }

    return true;
}

void RecommendController::renderRecommendResult(const RecommendParam& param, const std::vector<RecommendItem>& recItemVec)
{
    Value& resources = response()[Keys::resources];

    for (std::vector<RecommendItem>::const_iterator recIt = recItemVec.begin();
        recIt != recItemVec.end(); ++recIt)
    {
        Value& itemValue = resources();
        itemValue[Keys::weight] = recIt->weight_;

        const Document& item = recIt->item_;
        Document::property_const_iterator endIt = item.propertyEnd();
        for (Document::property_const_iterator it = item.propertyBegin();
            it != endIt; ++it)
        {
            const std::string& propName = docIdToItemId(it->first);
            renderPropertyValue(it->second, itemValue[propName]);
        }

        const std::vector<ReasonItem>& reasonItems = recIt->reasonItems_;
        // BAB need not reason results
        if (param.type != BUY_ALSO_BUY && reasonItems.empty() == false)
        {
            Value& reasonsValue = itemValue[Keys::reasons];
            for (std::vector<ReasonItem>::const_iterator it = reasonItems.begin();
                it != reasonItems.end(); ++it)
            {
                Value& value = reasonsValue();
                value[Keys::event] = it->event_;

                const Document& reasonItem = it->item_;
                Document::property_const_iterator findIt = reasonItem.findProperty(DOCID);
                if (findIt != reasonItem.propertyEnd())
                {
                    renderPropertyValue(findIt->second, value[ITEMID]);
                }

                if(! it->value_.empty())
                {
                    value[Keys::value] = it->value_;
                }
            }
        }
    }
}

/**
 * @brief Action @b top_item_bundle. Get the most frequent item bundles, with each bundle containing the items frequently bought together in one order.@n
 * To use this API, you need to configure @c enable in <tt> <RecommendBundle><Parameter><FreqItemSet></tt> to @c yes in config file (such as <tt>config/sf1config.xml</tt>).
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
 *     - each item properties would also be included here, such as "name", "link", etc.@n
 *       Property key name is used as key. The corresponding value is the content of the property.
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
    RecommendSearchService* service = collectionHandler_->recommendSearchService_;
    IZENELIB_DRIVER_BEFORE_HOOK(requireService(service));

    TIBParam tibParam;
    if (! parseTIBParam(tibParam))
        return;

    std::vector<ItemBundle> bundleVec;
    if (service->topItemBundle(tibParam, bundleVec))
    {
        renderBundleResult(bundleVec);
    }
    else
    {
        response().addError("Failed to get top item bundle from given collection.");
    }
}

bool RecommendController::parseTIBParam(TIBParam& param)
{
    izenelib::driver::Value& resourceValue = request()[Keys::resource];

    param.limit = asUintOr(resourceValue[Keys::max_count],
                           kDefaultRecommendCount);
    param.minFreq = asUintOr(resourceValue[Keys::min_freq],
                             kDefaultMinFreq);

    std::string errorMsg;
    if (! param.check(errorMsg))
    {
        response().addError(errorMsg);
        return false;
    }

    return true;
}

void RecommendController::renderBundleResult(const std::vector<ItemBundle>& bundleVec)
{
    Value& resources = response()[Keys::resources];

    for (std::vector<ItemBundle>::const_iterator bundleIt = bundleVec.begin();
        bundleIt != bundleVec.end(); ++bundleIt)
    {
        Value& bundleValue = resources();
        bundleValue[Keys::freq] = bundleIt->freq;

        Value& itemsValue = bundleValue[Keys::items];
        const std::vector<Document>& items = bundleIt->items;
        for (std::vector<Document>::const_iterator itemIt = items.begin();
            itemIt != items.end(); ++itemIt)
        {
            Value& itemValue = itemsValue();
            const Document& item = *itemIt;
            Document::property_const_iterator endIt = item.propertyEnd();
            for (Document::property_const_iterator it = item.propertyBegin();
                it != endIt; ++it)
            {
                const std::string& propName = docIdToItemId(it->first);
                renderPropertyValue(it->second, itemValue[propName]);
            }
        }
    }
}

} // namespace sf1r
