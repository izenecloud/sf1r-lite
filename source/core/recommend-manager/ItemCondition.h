/**
 * @file ItemCondition.h
 * @author Jun Jiang
 * @date 2011-06-01
 */

#ifndef ITEM_CONDITION_H
#define ITEM_CONDITION_H

#include "Item.h"

#include <util/ustring/UString.h>

#include <string>
#include <set>

namespace sf1r
{

struct ItemCondition
{
    /** property name */
    std::string propName_;

    /** property values set */
    typedef std::set<izenelib::util::UString> PropValueSet;
    PropValueSet propValueSet_;

    /**
     * Check whether the property value of @p item is contained in @c propValueSet_.
     * @param item the item to check
     * @return true for the item meets condition, false for not meet condition.
     */
    bool checkItem(const Item& item) const
    {
        izenelib::util::UString propValue;
        if (propName_ == "ITEMID")
        {
            propValue.assign(item.idStr_, izenelib::util::UString::UTF_8);
        }
        else
        {
            Item::PropValueMap::const_iterator it = item.propValueMap_.find(propName_);
            if (it != item.propValueMap_.end())
                propValue = it->second;
        }

        return propValueSet_.find(propValue) != propValueSet_.end();
    }
};

} // namespace sf1r

#endif // ITEM_CONDITION_H
