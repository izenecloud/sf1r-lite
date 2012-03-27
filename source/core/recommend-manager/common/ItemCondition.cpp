#include "ItemCondition.h"
#include "../item/ItemManager.h"
#include <document-manager/Document.h>

namespace sf1r
{

ItemCondition::ItemCondition()
    : itemManager_(NULL)
{
}

bool ItemCondition::isMeetCondition(itemid_t itemId) const
{
    if (! itemManager_)
        return false;

    // no condition
    if (propName_.empty())
        return itemManager_->hasItem(itemId);

    Document item;
    std::vector<std::string> propList;
    propList.push_back(propName_);

    // not exist
    if (! itemManager_->getItem(itemId, propList, item))
        return false;

    // compare property value
    Document::property_const_iterator it = item.findProperty(propName_);
    if (it != item.propertyEnd())
    {
        const izenelib::util::UString& propValue = it->second.get<izenelib::util::UString>();
        return propValueSet_.find(propValue) != propValueSet_.end();
    }

    return false;
}

} // namespace sf1r
