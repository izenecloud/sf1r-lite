/**
 * @file core/common/parsers/RangeParser.cpp
 * @author Jun Jiang
 * @date Created <2011-08-26>
 */
#include "RangeParser.h"

#include <common/ValueConverter.h>
#include <common/BundleSchemaHelpers.h>
#include <common/Keys.h>

namespace sf1r {
using driver::Keys;

/**
 * @class RangeParser
 *
 * Field @b range is an object. It specifies which property need range result (min, max value).
 *
 * Every item is an object having following fields.
 * - @b property* (@c String): Property which range result would be supplied in response["range"]
 *
 * The property type must be int or float.
 */

bool RangeParser::parse(const Value& rangeValue)
{
    clearMessages();
    rangeProperty_.clear();

    if (nullValue(rangeValue))
    {
        return true;
    }

    std::string property = asString(rangeValue[Keys::property]);

    PropertyConfig propertyConfig;
    propertyConfig.setName(property);
    if (!getPropertyConfig(indexSchema_, propertyConfig))
    {
        error() = "Unknown property in range[property]: " + property;
        return false;
    }

    if (propertyConfig.bIndex_ && propertyConfig.bFilter_
        && (propertyConfig.propertyType_ == INT32_PROPERTY_TYPE
           || propertyConfig.propertyType_ == DATETIME_PROPERTY_TYPE
           || propertyConfig.propertyType_ == FLOAT_PROPERTY_TYPE
           || propertyConfig.propertyType_ == INT64_PROPERTY_TYPE))
    {
        rangeProperty_ = property;
    }
    else
    {
        warning() = "Ignore range[property]: " + property
                    + ", as it should be configured as filter property of int or float type";
    }

    return true;
}

} // namespace sf1r
