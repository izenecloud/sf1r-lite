/**
 * @file ItemManager.h
 * @author Jun Jiang
 * @date 2011-04-19
 */

#ifndef ITEM_MANAGER_H
#define ITEM_MANAGER_H

#include "../common/RecTypes.h"

namespace sf1r
{

class DocumentManager;
class Document;

class ItemManager
{
public:
    ItemManager(DocumentManager* docManager);

    bool getItem(itemid_t itemId, Document& doc);

    bool hasItem(itemid_t itemId) const;

    itemid_t maxItemId() const;

private:
    DocumentManager* docManager_;
};

} // namespace sf1r

#endif // ITEM_MANAGER_H
