/**
 * @file AttrParser.cpp
 * @author Jun Jiang
 * @date Created <2011-06-25>
 */
#include "AttrParser.h"

#include <common/ValueConverter.h>
#include <common/BundleSchemaHelpers.h>
#include <common/Keys.h>

namespace sf1r {
using driver::Keys;

bool AttrParser::parse(const Value& attrValue)
{
    clearMessages();

    if (nullValue(attrValue))
        return true;

    if (attrValue.type() != Value::kObjectType)
    {
        error() = "Require an object for request[\"attr\"].";
        return false;
    }

    if (attrValue.hasKey(Keys::attr_result))
    {
        attrResult_ = asBool(attrValue[Keys::attr_result]);
    }

    if (attrValue.hasKey(Keys::attr_top))
    {
        attrTop_ = asUint(attrValue[Keys::attr_top]);
    }

    return true;
}

} // namespace sf1r
