/**
 * @file CartManager.h
 * @author Jun Jiang
 * @date 2011-08-05
 */

#ifndef CART_MANAGER_H
#define CART_MANAGER_H

#include "RecTypes.h"
#include <sdb/SequentialDB.h>

#include <string>
#include <vector>

namespace sf1r
{
class ItemManager;

class CartManager
{
public:
    CartManager(const std::string& path);

    ~CartManager();

    void flush();

    /**
     * Add a shopping cart update event.
     * @param userId the user id
     * @param itemVec the items in the shopping cart
     * @return true for succcess, false for failure
     */
    bool updateCart(
        userid_t userId,
        const std::vector<itemid_t>& itemVec
    );

    /**
     * Get the shopping cart items of @p userId into @p itemVec.
     * @param userId user id
     * @param itemVec store the item ids in shopping cart
     * @return true for success, false for error happened.
     */
    bool getCart(userid_t userId, std::vector<itemid_t>& itemVec);


private:
    typedef izenelib::sdb::unordered_sdb_tc<userid_t, std::vector<itemid_t>, ReadWriteLock> SDBType;
    SDBType container_;
};

} // namespace sf1r

#endif // CART_MANAGER_H
