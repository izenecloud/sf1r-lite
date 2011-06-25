/**
 * @file AttrParser.cpp
 * @author Jun Jiang
 * @date Created <2011-06-25>
 */
#include "AttrParser.h"

#include <common/ValueConverter.h>
#include <common/IndexBundleSchemaHelpers.h>
#include <common/Keys.h>

namespace sf1r {
using driver::Keys;

bool AttrParser::parse(const Value& attrValue)
{
    clearMessages();

    if (nullValue(attrValue))
    {
        return true;
    }

    if (attrValue.type() == Value::kObjectType)
    {
        if (attrValue.hasKey(Keys::attr_result))
        {
            attrResult_ = asBool(attrValue[Keys::attr_result]);
        }

        if (attrValue.hasKey(Keys::attr_top))
        {
            attrTop_ = asUint(attrValue[Keys::attr_top]);
        }
    }
    else
    {
        error() = "Require an object for request[\"attr\"].";
        return false;
    }

    if (attrResult_ && !miningSchema_.group_enable)
    {
        error() = "To get group results by attribute value, the attribute property should be configured in <MiningBundle>::<Schema>::<Attr>.";
        return false;
    }

    return true;
}

} // namespace sf1r
