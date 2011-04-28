/**
 * @file ItemManager.h
 * @author Jun Jiang
 * @date 2011-04-19
 */

#ifndef ITEM_MANAGER_H
#define ITEM_MANAGER_H

#include "RecTypes.h"
#include "Item.h"
#include <sdb/SequentialDB.h>
#include <sdb/SDBCursorIterator.h>

#include <string>

namespace sf1r
{

class ItemManager
{
public:
    ItemManager(const std::string& path);

    void flush();

    bool addItem(itemid_t itemId, const Item& item);
    bool updateItem(itemid_t itemId, const Item& item);
    bool removeItem(itemid_t itemId);

    bool hasItem(itemid_t itemId);
    bool getItem(itemid_t itemId, Item& item);
    unsigned int itemNum();

    typedef izenelib::sdb::unordered_sdb_tc<itemid_t, Item, ReadWriteLock> SDBType;
    typedef izenelib::sdb::SDBCursorIterator<SDBType> SDBIterator;
    SDBIterator begin();
    SDBIterator end();

private:
    SDBType container_;
};

} // namespace sf1r

#endif // ITEM_MANAGER_H
