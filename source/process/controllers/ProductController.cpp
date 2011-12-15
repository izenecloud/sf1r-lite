#include <bundles/product/ProductSearchService.h>
#include "ProductController.h"
#include <product-manager/product_manager.h>
#include <common/Utilities.h>
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
    if (!collectionHandler_->productSearchService_)
    {
        response().addError("ProductManager not enabled.");
        return false;
    }
    product_manager_ = collectionHandler_->productSearchService_->GetProductManager();

    if (!product_manager_)
    {
        response().addError("ProductManager not enabled.");
        return false;
    }
    return true;
}

bool ProductController::require_docid_list_()
{
    Value& resources = request()[Keys::docid_list];
    for (uint32_t i = 0; i < resources.size(); i++)
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

bool ProductController::require_str_docid_list_()
{
    Value& resources = request()[Keys::docid_list];
    for (uint32_t i = 0; i < resources.size(); i++)
    {
        Value& resource = resources(i);
        std::string str_docid = asString(resource);
        str_docid_list_.push_back(str_docid);
    }
    if (str_docid_list_.empty())
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
    if (suuid.empty())
    {
        response().addError("Require uuid in request.");
        return false;
    }
    uuid_ = izenelib::util::UString(suuid, izenelib::util::UString::UTF_8);
    return true;
}

bool ProductController::require_option_()
{
    Value& option = request()[Keys::option];
    if( izenelib::driver::nullValue(option) )
    {
        response().addError("Require option in request.");
        return false;
    }
    Value& force = option[Keys::force];
    if( izenelib::driver::nullValue(force) )
    {
        response().addError("Require force in option request.");
        return false;
    }
    option_.force = asBool(force);
    return true;
}

bool ProductController::maybe_doc_(bool must)
{
    doc_.clear();
    Value& resource = request()[Keys::resource];
    const Value::ObjectType& objectValue = resource.getObject();

    for (Value::ObjectType::const_iterator it = objectValue.begin();
         it != objectValue.end(); ++it)
    {
        std::string pname = it->first;
        izenelib::util::UString pvalue(asString(it->second), izenelib::util::UString::UTF_8);
        doc_.property(pname) = pvalue;
    }
    if( doc_.getPropertySize()==0 ) //empty resource doc
    {
        if(must)
        {
            response().addError("Require non-empty resource property in request.");
            return false;
        }
        else
        {
            return true;
        }
    }
    const PMConfig& config = product_manager_->GetConfig();
    //check validation
    if (!doc_.hasProperty(config.docid_property_name))
    {
        response().addError("Require DOCID property in request.");
        return false;
    }
    if (doc_.hasProperty(config.price_property_name))
    {
        response().addError("Can not update Price property manually.");
        return false;
    }
    if (doc_.hasProperty(config.itemcount_property_name))
    {
        response().addError("Can not update itemcount property manually.");
        return false;
    }
    return true;
}

bool ProductController::require_date_range_()
{
    if (!izenelib::driver::nullValue(request()[Keys::date_range]))
    {
        Value& date_range = request()[Keys::date_range];
        from_date_ = izenelib::util::UString(asString(date_range[Keys::start]), izenelib::util::UString::UTF_8);
        to_date_ = izenelib::util::UString(asString(date_range[Keys::end]), izenelib::util::UString::UTF_8);
    }
    else
    {
        response().addError("Require field date_range in request.");
        return false;
    }

    if ((from_tt_ = Utilities::createTimeStamp(from_date_)) == -2)
    {
        response().addError("start date not valid.");
        return false;
    }
    if ((to_tt_ = Utilities::createTimeStamp(to_date_)) == -2)
    {
        response().addError("end date not valid.");
        return false;
    }
    if (to_tt_ != -1)
    {
        if (from_tt_ > to_tt_)
        {
            response().addError("date range not valid.");
            return false;
        }
        to_tt_ += 86399999999L;
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
 * - @b resource (@c Object): The corresponding information for the grouped documents, you can specify title, content.. etc, for it.
 *   Property key name is used as
 *   key. The corresponding value is the content of the property. Property @b
 *   DOCID is optional, you can specify a particular DOCID which will be used as uuid.
 * - @b option*  (@c Object): Edit options
 *   - @b force* (@c Bool = @c false): if true, ignore the conflict while editing.
 *
 * @section response
 *
 * - @b uuid (@c String): Generated or specified uuid.
 *
 *
 * @section Example
 *
 * Request
 * @code
 * {
 *   "collection" : "b5mm",
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
    IZENELIB_DRIVER_BEFORE_HOOK(require_docid_list_());
    IZENELIB_DRIVER_BEFORE_HOOK(maybe_doc_(false));
    IZENELIB_DRIVER_BEFORE_HOOK(require_option_());
    
    if (!product_manager_->AddGroup(docid_list_, doc_, option_))
    {
        response().addError(product_manager_->GetLastError());
        return;
    }
    const PMConfig& config = product_manager_->GetConfig();
    izenelib::util::UString uuid;
    doc_.getProperty(config.docid_property_name, uuid);
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
 * - @b option*  (@c Object): Edit options
 *   - @b force* (@c Bool = @c false): if true, ignore the conflict while editing.
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
 *   "collection" : "b5mm",
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
    IZENELIB_DRIVER_BEFORE_HOOK(require_docid_list_());
    IZENELIB_DRIVER_BEFORE_HOOK(require_option_());
    if (!product_manager_->AppendToGroup(uuid_, docid_list_, option_))
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
 * - @b option*  (@c Object): Edit options
 *   - @b force* (@c Bool = @c false): if true, ignore the conflict while editing.
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
 *   "collection" : "b5mm",
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
    IZENELIB_DRIVER_BEFORE_HOOK(require_docid_list_());
    IZENELIB_DRIVER_BEFORE_HOOK(require_option_());
    if (!product_manager_->RemoveFromGroup(uuid_, docid_list_, option_))
    {
        response().addError(product_manager_->GetLastError());
        return;
    }
}

/**
 * @brief Action \b recover. Recover product-manager data from backup
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
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
 *   "collection" : "b5mm",
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
// void ProductController::recover()
// {
//     IZENELIB_DRIVER_BEFORE_HOOK(check_product_manager_());
//     if (!product_manager_->Recover())
//     {
//         response().addError(product_manager_->GetLastError());
//         return;
//     }
// }

/**
 * @brief Action @b update_a_doc. Updates documents for A collection for product-manager
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name for 'M'(b5mm).
 * - @b resource* (@c Object): A document resource in 'A' collection. Property key name is used as
 *   key. The corresponding value is the content of the property. Property @b
 *   DOCID is required, which is a unique document identifier specified by
 *   client. This DOCID is always the uuid in 'M' collection.
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
 *     "DOCID": "xxx-yyy-zzz",
 *     "Title": "Hello, World (revision)",
 *     "Content": "This is a revised test post."
 *   }
 * }
 * @endcode
 */
void ProductController::update_a_doc()
{
    IZENELIB_DRIVER_BEFORE_HOOK(check_product_manager_());
    IZENELIB_DRIVER_BEFORE_HOOK(maybe_doc_(true));
    if (!product_manager_->UpdateADoc(doc_))
    {
        response().addError(product_manager_->GetLastError());
        return;
    }
}

/**
 * @brief Action \b get_multi_price_history. Get price history for given document id list and optional date range.
 *
 * @section request
 *
 * - @b collection* (@c String): Collection name.
 * - @b docid_list* (@c Array): string-type document id list
 * - @b date_range* (@c Object): specify the date range
 *   - @c start (@c String): start date string
 *   - @c end (@c String): end date string
 * - @b range* (@c Bool = @c false): indicate whether to return full history or lower and upper bounds
 *
 * @section response
 *
 * if range is not set
 *
 * - @b resources (@c Array): All returned items
 *   - @b docid (@c String): The string_type docid
 *   - @b price_history (@c Array): The price history
 *     - @b timestamp (@c String): The timestamp
 *     - @b price_range (@c Object): the price range
 *       - @c price_low (@c Double): the lowest price
 *       - @c price_high (@c Double): the highest price
 *
 * if range is set
 *
 * - @b resources (@c Array): All returned items
 *   - @b docid (@c String): The string_type docid
 *   - @b price_range (@c Object): the price range
 *     - @c price_low (@c Double): the lowest price
 *     - @c price_high (@c Double): the highest price
 *
 * @section Example
 *
 * Request
 * @code
 * {
 *   "collection":"b5mm",
 *   "docid_list":
 *   [
 *     "f16940a39c79dc84bf14b94dab0624fd",
 *     "cbb3c856bda004e3d9dd441a3995b090"
 *   ],
 *   "date_range":
 *   {
 *     "start":"2010-01-01",
 *     "end":""
 *   }
 * }
 * @endcode
 *
 * Response
 * @code
 * {
 *   "header":
 *   {
 *     "success":true
 *   },
 *   "resources":
 *   [
 *     {
 *       "docid":"cbb3c856bda004e3d9dd441a3995b090",
 *       "price_history":
 *       [
 *         {
 *           "timestamp":"20110908T170011.111000",
 *           "price_range":
 *           {
 *             "price_high":45,
 *             "price_low":45
 *           }
 *         }
 *       ]
 *     },
 *     {
 *       "docid":"f16940a39c79dc84bf14b94dab0624fd",
 *       "price_history":
 *       [
 *         {
 *           "timestamp":"20110908T170011.111000",
 *           "price_range":
 *           {
 *             "price_high":85,
 *             "price_low":85
 *           }
 *         }
 *       ]
 *     }
 *   ]
 * }
 * @endcode
 */
void ProductController::get_multi_price_history()
{
    IZENELIB_DRIVER_BEFORE_HOOK(check_product_manager_());
    IZENELIB_DRIVER_BEFORE_HOOK(require_str_docid_list_());
    IZENELIB_DRIVER_BEFORE_HOOK(require_date_range_());

    bool is_range = asBool(request()[Keys::range]);
    if (!is_range)
    {
        PriceHistoryList history_list;
        if (!product_manager_->GetMultiPriceHistory(history_list, str_docid_list_, from_tt_, to_tt_))
        {
            response().addError(product_manager_->GetLastError());
            return;
        }

        Value& price_history_list = response()[Keys::resources];
        for (Value::UintType i = 0; i < history_list.size(); ++i)
        {
            Value& doc_item = price_history_list();
            doc_item[Keys::docid] = history_list[i].first;
            Value& price_history = doc_item[Keys::price_history];
            PriceHistoryItem& history = history_list[i].second;
            for (Value::UintType j = 0; j < history.size(); ++j)
            {
                Value& history_item = price_history();
                history_item[Keys::timestamp] = history[j].first;
                Value& price_range = history_item[Keys::price_range];
                price_range[Keys::price_low] = history[j].second.first;
                price_range[Keys::price_high] = history[j].second.second;
            }
        }
    }
    else
    {
        PriceRangeList range_list;
        if (!product_manager_->GetMultiPriceRange(range_list, str_docid_list_, from_tt_, to_tt_))
        {
            response().addError(product_manager_->GetLastError());
            return;
        }

        Value& price_range_list = response()[Keys::resources];
        for (Value::UintType i = 0; i < range_list.size(); ++i)
        {
            Value& doc_item = price_range_list();
            doc_item[Keys::docid] = range_list[i].first;
            Value& price_range = doc_item[Keys::price_range];
            price_range[Keys::price_low] = range_list[i].second.first;
            price_range[Keys::price_high] = range_list[i].second.second;
        }
    }
}

/**
 * @brief Action \b get_top_price_cut_list. Get 20 doc ids with greatest price-cut ratio for given property value and time interval.
 *
 * @section request
 *
 * - @b collection* (@c String): collection name.
 * - @b property* (@c String): property name
 * - @c value* (@c String): property value
 * - @b days* (@c Uint): time interval to calculate price-cut ratio
 * - @b count (@c Uint): max count of returned products
 *
 * @section response
 *
 * - @b count (@c Uint): count of returned items
 * - @b resources (@c Array): all returned items
 *   - @b price_cut (@c Float): the price-cut ratio
 *   - @b docid (@c String): the string_type docid
 *
 * @section Example
 *
 * Request
 * @code
 * {
 *   "collection":"b5mm",
 *   "property":"Category",
 *   "value":"手机通讯",
 *   "days":"7"
 * }
 * @endcode
 *
 * Response
 * @code
 * {
 *   "header":
 *   {
 *     "success":true
 *   },
 *   "count":"2",
 *   "resources":
 *   [
 *     {
         "price_cut":"0.35",
 *       "docid":"cbb3c856bda004e3d9dd441a3995b090"
 *     },
 *     {
         "price_cut":"0.15",
 *       "docid":"f16940a39c79dc84bf14b94dab0624fd"
 *     }
 *   ]
 * }
 * @endcode
 */
void ProductController::get_top_price_cut_list()
{
    IZENELIB_DRIVER_BEFORE_HOOK(check_product_manager_());

    prop_name_ = asString(request()[Keys::property]);
    prop_value_ = asString(request()[Keys::value]);
    days_ = asUint(request()[Keys::days]);
    if (!izenelib::driver::nullValue(request()[Keys::count]))
        count_ = asUint(request()[Keys::count]);
    else
        count_ = 20;

    TPCQueue tpc_queue;
    if (!product_manager_->GetTopPriceCutList(tpc_queue, prop_name_, prop_value_, days_, count_))
    {
        response().addError(product_manager_->GetLastError());
        return;
    }

    response()[Keys::count] = tpc_queue.size();
    Value& tpc_list = response()[Keys::resources];
    for (uint32_t i = 0; i < tpc_queue.size(); i++)
    {
        Value& tpc_item = tpc_list();
        tpc_item[Keys::price_cut] = tpc_queue[i].first;
        tpc_item[Keys::docid] = tpc_queue[i].second;
    }
}

}
