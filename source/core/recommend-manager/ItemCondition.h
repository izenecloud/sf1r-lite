/**
 * @file ItemCondition.h
 * @author Jun Jiang
 * @date 2011-06-01
 */

#ifndef ITEM_CONDITION_H
#define ITEM_CONDITION_H

#include <util/ustring/UString.h>

#include <string>
#include <set>

namespace sf1r
{
class Document;

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
    bool checkItem(const Document& item) const;
};

} // namespace sf1r

#endif // ITEM_CONDITION_H
