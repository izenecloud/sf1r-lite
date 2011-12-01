/**
 * @file core/common/parsers/SortParser.cpp
 * @author Ian Yang
 * @date Created <2010-07-12 12:19:35>
 */
#include "SortParser.h"

#include <common/IndexBundleSchemaHelpers.h>

#include <query-manager/QueryManager.h>

#include <boost/algorithm/string/case_conv.hpp>

namespace sf1r {

/**
 * @class SortParser
 *
 * Field \b sort is an array, which specifies how to sort the result by
 * properties.
 *
 * Every item is a String or an Object.
 *
 * If the item is a String, it is used as the property name and the result
 * should be sorted by that property in ascending order.
 *
 * If it is an Object:
 * - @b property* (@c String): Property name that should be sorted by.
 * - @b order (@c String = "ASC"): Specify the order using keywords:
 *   - ASC : sort in ascending order
 *   - DESC : sort in descending order.
 *
 * The property must be sortable. Check field indices in schema/get (see
 * SchemaController::get() ). All index names (the @b name field in every
 * index) which @b filter is @c true and corresponding property type is int or
 * float can be used here.
 *
 * The result are first sorted by the first item, then the second and so on.
 */

bool SortParser::parse(const Value& orders)
{
    OrderArrayParser::parse(orders);

    sortPriorityList_.resize(parsedOrderCount());
    for (std::size_t i = 0; i < parsedOrderCount(); ++i)
    {
        if (parsedOrders(i).property() == "_ctr")
        {
            //nothing
        }
        else if (parsedOrders(i).property() == "custom_rank")
        {
            if (!bCustomRank_) {
                error() = "Please make sure \"custom_rank\" field is available, which is set as a sort property.";
                return false;
            }
        }
        else if (!isPropertySortable(indexSchema_, parsedOrders(i).property()))
        {
            error() = "Property is not sortable: " + parsedOrders(i).property();
            return false;
        }

        sortPriorityList_[i].first = parsedOrders(i).property();
        sortPriorityList_[i].second = parsedOrders(i).ascendant();
    }

    // sort by custom rank if empty and custom rank available
    if (sortPriorityList_.empty() && bCustomRank_)
    {
        static const std::pair<std::string , bool> kDefaultOrder("custom_rank", false);
        sortPriorityList_.push_back(kDefaultOrder);
    }

    // sort by RANK if empty
    if (sortPriorityList_.empty())
    {
        static const std::pair<std::string , bool> kDefaultOrder("_rank", false);
        sortPriorityList_.push_back(kDefaultOrder);
    }

    return true;
}

} // namespace sf1r


