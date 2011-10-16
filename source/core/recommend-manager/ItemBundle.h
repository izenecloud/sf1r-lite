/**
 * @file ItemBundle.h
 * @brief recommend result of "top item bundle" 
 * @author Jun Jiang
 * @date 2011-10-16
 */

#ifndef ITEM_BUNDLE_H
#define ITEM_BUNDLE_H

#include "Item.h"
#include "RecTypes.h"

#include <vector>

namespace sf1r
{

struct ItemBundle
{
    ItemBundle() : freq(0) {}

    int freq;
    std::vector<Item> items;
    std::vector<itemid_t> itemIds;
};

} // namespace sf1r

#endif // ITEM_BUNDLE_H
