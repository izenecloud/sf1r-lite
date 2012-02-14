/**
 * @file ItemManager.h
 * @author Jun Jiang
 * @date 2012-02-14
 */

#ifndef ITEM_MANAGER_H
#define ITEM_MANAGER_H

#include "../common/RecTypes.h"

namespace sf1r
{
class Document;

class ItemManager
{
public:
    virtual ~ItemManager() {}

    virtual bool getItem(itemid_t itemId, Document& doc) = 0;

    virtual bool hasItem(itemid_t itemId) const = 0;
};

} // namespace sf1r

#endif // ITEM_MANAGER_H
