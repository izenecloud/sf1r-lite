/**
 * @file LocalPurchaseManager.h
 * @author Jun Jiang
 * @date 2011-04-19
 */

#ifndef LOCAL_PURCHASE_MANAGER_H
#define LOCAL_PURCHASE_MANAGER_H

#include "PurchaseManager.h"
#include <sdb/SequentialDB.h>

#include <string>
#include <boost/serialization/set.hpp> // serialize ItemIdSet

namespace sf1r
{

class LocalPurchaseManager : public PurchaseManager
{
public:
    LocalPurchaseManager(const std::string& path);

    virtual void flush();

    virtual bool getPurchaseItemSet(
        const std::string& userId,
        ItemIdSet& itemIdSet
    );

protected:
    virtual bool savePurchaseItem_(
        const std::string& userId,
        const ItemIdSet& totalItems,
        const std::list<itemid_t>& newItems
    );

private:
    typedef izenelib::sdb::unordered_sdb_tc<std::string, ItemIdSet, ReadWriteLock> SDBType;
    SDBType container_;
};

} // namespace sf1r

#endif // LOCAL_PURCHASE_MANAGER_H
