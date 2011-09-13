/**
 * @file core/common/parsers/GroupingParser.cpp
 * @author Ian Yang
 * @date Created <2010-06-12 09:08:30>
 */
#include "GroupingParser.h"

#include <common/ValueConverter.h>
#include <common/IndexBundleSchemaHelpers.h>
#include <common/Keys.h>

namespace sf1r {
using driver::Keys;

/**
 * @class GroupingParser
 *
 * Field @b group is an array. It specifies which properties need doc count result for each property value.
 *
 * Every item is an object having following fields.
 * - @b property* (@c String): Property name, whose group result (doc count for
 *   each property value) would be supplied in response["group"].@n
 *   The property type must be string, int or float.
 * - @b range (@c Bool = @c false): For the property type of int or float, you
 *   could also set this parameter to true,@n
 *   then the group results would be in range form such as "101-200",
 *   meaning the values contained in this range, including both boundary values.
 */

bool GroupingParser::parse(const Value& grouping)
{
    clearMessages();
    propertyList_.clear();

    if (nullValue(grouping))
    {
        return true;
    }

    if (!miningSchema_.group_enable)
    {
        error() = "The GroupBy properties have not been configured in <MiningBundle>::<Schema>::<Group> yet.";
        return false;
    }

    if (grouping.type() != Value::kArrayType)
    {
        error() = "Require an array for parameter [group].";
        return false;
    }

    const std::vector<GroupConfig> groupProps = miningSchema_.group_properties;
    for (std::size_t i = 0; i < grouping.size(); ++i)
    {
        const Value& groupingRule = grouping(i);
        faceted::GroupPropParam propParam;

        if (groupingRule.type() == Value::kObjectType)
        {
            propParam.property_ = asString(groupingRule[Keys::property]);
            if (groupingRule.hasKey(Keys::range))
            {
                propParam.isRange_ = asBool(groupingRule[Keys::range]);
            }
        }
        else
        {
            propParam.property_ = asString(groupingRule);
        }

        const std::string& propName = propParam.property_;
        std::vector<GroupConfig>::const_iterator configIt;
        for (configIt = groupProps.begin(); configIt != groupProps.end(); ++configIt)
        {
            if (configIt->propName == propName)
                break;
        }

        if (configIt == groupProps.end())
        {
            error() = "\"" + propName + "\" is not GroupBy property, it should be configured in <MiningBundle>::<Schema>::<Group>.";
            return false;
        }

        if (propParam.isRange_)
        {
            if (!configIt->isNumericType())
            {
                error() = "the property type of \"" + propName + "\" is not int or float for range group.";
                return false;
            }
        }
        else
        {
            if (!configIt->isStringType() || !configIt->isNumericType())
            {
                error() = "the property type of \"" + propName + "\" is not string, int or float for group.";
                return false;
            }
        }

        propertyList_.push_back(propParam);
    }

    return true;
}

} // namespace sf1r
