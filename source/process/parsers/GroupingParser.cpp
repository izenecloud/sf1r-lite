/**
 * @file core/common/parsers/GroupingParser.cpp
 * @author Ian Yang
 * @date Created <2010-06-12 09:08:30>
 */
#include "GroupingParser.h"

#include <common/ValueConverter.h>
#include <common/BundleSchemaHelpers.h>
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
 * - @b sub_property (@c String): Property name, it gives 2nd level group results
 *   in response["group"]["labels"]["sub_labels"].@n
 *   The property type must be string, int or float. If this parameter is set, @b range must be false.
 */

bool GroupingParser::parse(const Value& grouping)
{
    clearMessages();
    propertyList_.clear();

    if (nullValue(grouping))
        return true;

    if (grouping.type() != Value::kArrayType)
    {
        error() = "Require an array for parameter [group].";
        return false;
    }

    for (std::size_t i = 0; i < grouping.size(); ++i)
    {
        const Value& groupingRule = grouping(i);
        faceted::GroupPropParam propParam;

        if (groupingRule.type() == Value::kObjectType)
        {
            propParam.property_ = asString(groupingRule[Keys::property]);
            propParam.subProperty_ = asString(groupingRule[Keys::sub_property]);
            if (groupingRule.hasKey(Keys::range))
            {
                propParam.isRange_ = asBool(groupingRule[Keys::range]);
            }
        }
        else
        {
            propParam.property_ = asString(groupingRule);
        }

        propertyList_.push_back(propParam);
    }

    return true;
}

} // namespace sf1r
