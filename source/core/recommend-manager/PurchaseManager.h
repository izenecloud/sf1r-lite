/**
 * @file PurchaseManager.h
 * @author Jun Jiang
 * @date 2011-04-19
 */

#ifndef PURCHASE_MANAGER_H
#define PURCHASE_MANAGER_H

#include "RecTypes.h"
#include <sdb/SequentialDB.h>

#include <string>
#include <vector>

#include <boost/serialization/set.hpp> // serialize ItemIdSet

namespace sf1r
{
class RecommendMatrix;

class PurchaseManager
{
public:
    PurchaseManager(const std::string& path);

    ~PurchaseManager();

    void flush();

    /**
     * Add a purchase event.
     * @param userId the user id
     * @param itemVec the items in the order
     * @param matrix if not NULL, @c RecommendMatrix::update() would be called
     * @return true for succcess, false for failure
     */
    bool addPurchaseItem(
        const std::string& userId,
        const std::vector<itemid_t>& itemVec,
        RecommendMatrix* matrix
    );

    /**
     * Get @p itemIdSet purchased by @p userId.
     * @param userId user id
     * @param itemidSet item id set purchased by @p userId, it would be empty if @p userId
     *                  has not purchased any item.
     * @return true for success, false for error happened.
     */
    bool getPurchaseItemSet(const std::string& userId, ItemIdSet& itemIdSet);

private:
    typedef izenelib::sdb::unordered_sdb_tc<std::string, ItemIdSet, ReadWriteLock> SDBType;
    SDBType container_;
};

} // namespace sf1r

#endif // PURCHASE_MANAGER_H
