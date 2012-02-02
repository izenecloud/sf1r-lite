/**
 * @file CartManager.h
 * @author Jun Jiang
 * @date 2012-02-01
 */

#ifndef CART_MANAGER_H
#define CART_MANAGER_H

#include <recommend-manager/RecTypes.h>

#include <string>
#include <vector>

namespace sf1r
{

class CartManager
{
public:
    virtual ~CartManager() {}

    virtual void flush() {}

    /**
     * Add a shopping cart update event.
     * @param userId the user id
     * @param itemVec the items in the shopping cart
     * @return true for succcess, false for failure
     */
    virtual bool updateCart(
        const std::string& userId,
        const std::vector<itemid_t>& itemVec
    ) = 0;

    /**
     * Get the shopping cart items of @p userId into @p itemVec.
     * @param userId user id
     * @param itemVec store the item ids in shopping cart
     * @return true for success, false for error happened.
     */
    virtual bool getCart(
        const std::string& userId,
        std::vector<itemid_t>& itemVec
    ) = 0;
};

} // namespace sf1r

#endif // CART_MANAGER_H
