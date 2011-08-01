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
 * Field @b group is an array. It specifies which properties need statistics info (doc count) for each property value.
 *
 * Every item is an object having following fields.
 * - @b property* (@c String): Property which statistics info would be supplied in response["group"]
 *
 * The property type must be string, int or float.
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

    if (grouping.type() == Value::kArrayType)
    {
        propertyList_.resize(grouping.size());

        const std::vector<GroupConfig> groupProps = miningSchema_.group_properties;
        for (std::size_t i = 0; i < grouping.size(); ++i)
        {
            const Value& groupingRule = grouping(i);
            std::string& property = propertyList_[i];

            if (groupingRule.type() == Value::kObjectType)
            {
                property = asString(groupingRule[Keys::property]);
            }
            else
            {
                property = asString(groupingRule);
            }

            std::vector<GroupConfig>::const_iterator configIt =
                std::find_if(groupProps.begin(), groupProps.end(),
                    boost::bind(&GroupConfig::propName, _1) == property);

            if (configIt == groupProps.end())
            {
                error() = "\"" + property + "\" is not GroupBy property, it should be configured in <MiningBundle>::<Schema>::<Group>.";
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

} // namespace sf1r
