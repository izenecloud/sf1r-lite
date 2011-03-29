/**
 * @file core/common/parsers/GroupingParser.cpp
 * @author Ian Yang
 * @date Created <2010-06-12 09:08:30>
 */
#include "GroupingParser.h"

#include <common/ValueConverter.h>
#include <common/IndexBundleSchemaHelpers.h>
#include <common/Keys.h>

#include <query-manager/ActionItem.h>

namespace sf1r {
using driver::Keys;

/**
 * @class GroupingParser
 *
 * Field @b group is an array. It specifies how to organize result into groups.
 *
 * Every item is an object having following fields.
 * - @b property* (@c String): Property that result should be grouped by.
 * - @b group_count (@c Uint): Specify number of group counts. This is required
 *   for ranged grouping.
 * - @b from (@c Any): Start value (inclusive) for ranged grouping.
 * - @b to (@c Any): End value (inclusive) for ranged grouping.
 *
 * The property must be sortable. Check field indices in schema/get (see
 * SchemaController::get() ). All index names (the @b name field in every
 * index) which @b filter is @c true and corresponding property type is int or
 * float can be used here.
 *
 * If only @b property is given, result is grouped by that property. Only
 * results having the same value of that property are in the same group.
 *
 * If @b group_count, @b from and @b to are specified, the range [@b from, @b
 * to] are split into @b group_count equal sub-ranges. Results in the same
 * sub-range are in the same group.
 */

bool GroupingParser::parse(const Value& grouping)
{
    clearMessages();
    options_.clear();

    if (nullValue(grouping))
    {
        return true;
    }

    if (grouping.type() == Value::kArrayType)
    {
        options_.resize(grouping.size());
        for (std::size_t i = 0; i < grouping.size(); ++i)
        {
            if (!parse(grouping(i), options_[i]))
            {
                return false;
            }
        }
    }
    else
    {
        error() = "Require an array for grouping rules.";
        return false;
    }

    return true;
}

bool GroupingParser::parse(
                           const Value& groupingRule,
                           GroupingOption& option)
{
    option.num_ = 0;
    if (groupingRule.type() == Value::kObjectType)
    {
        option.propertyString_ = asString(groupingRule[Keys::property]);
    }
    else
    {
        option.propertyString_ = asString(groupingRule);
    }


//    if (!isPropertySortable(meta, option.propertyString_))
//    {
//        error() = "Property is not sortable and cannot be used in group: " +
//                  option.propertyString_;
//        return false;
//    }

    if (groupingRule.type() == Value::kObjectType)
    {
        option.num_ = asUint(groupingRule[Keys::group_count]);
        if (option.num_ > 0)
        {
            if (groupingRule.hasKey(Keys::from) && groupingRule.hasKey(Keys::to))
            {
                sf1r::PropertyDataType dataType =
                    getPropertyDataType(indexSchema_, option.propertyString_);
                if (dataType == sf1r::UNKNOWN_DATA_PROPERTY_TYPE)
                {
                    error() = "Property's data type is unknown: " +
                              option.propertyString_;
                    return false;
                }

                ValueConverter::driverValue2PropertyValue(
                    dataType, groupingRule[Keys::from], option.from_
                );
                ValueConverter::driverValue2PropertyValue(
                    dataType, groupingRule[Keys::to], option.to_
                );
            }
            else
            {
                error() = "Group count is given, "
                          "but from and to are not specified.";
                return false;
            }
        }
    }

    return true;
}

} // namespace sf1r
