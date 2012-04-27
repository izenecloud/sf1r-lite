/**
 * @file ItemCondition.h
 * @author Jun Jiang
 * @date 2011-06-01
 */

#ifndef ITEM_CONDITION_H
#define ITEM_CONDITION_H

#include "RecTypes.h"
#include <util/ustring/UString.h>

#include <string>
#include <set>

namespace sf1r
{
class ItemManager;

struct ItemCondition
{
    ItemCondition();

    /** property name */
    std::string propName_;

    /** property values set */
    typedef std::set<izenelib::util::UString> PropValueSet;
    PropValueSet propValueSet_;

    ItemManager* itemManager_;

    /**
     * Check whether @p itemId meets the condition.
     * @param item the item to check
     * @return true for the item meets condition, false for not meet condition.
     */
    bool isMeetCondition(itemid_t itemId) const;
};

} // namespace sf1r

#endif // ITEM_CONDITION_H
