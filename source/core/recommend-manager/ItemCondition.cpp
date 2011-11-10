#include "ItemCondition.h"
#include <document-manager/Document.h>

namespace sf1r
{

bool ItemCondition::checkItem(const Document& item) const
{
    Document::property_const_iterator it = item.findProperty(propName_);
    if (it != item.propertyEnd())
    {
        const izenelib::util::UString& propValue = it->second.get<izenelib::util::UString>();
        return propValueSet_.find(propValue) != propValueSet_.end();
    }

    return false;
}
} // namespace sf1r
