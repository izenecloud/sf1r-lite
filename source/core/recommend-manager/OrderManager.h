/**
 * @file OrderManager.h
 * @author Yingfeng Zhang
 * @date 2011-04-19
 */

#ifndef ORDER_MANAGER_H
#define ORDER_MANAGER_H

#include "RecTypes.h"
#include "ItemIndex.h"

#include <string>
#include <vector>

#include <boost/serialization/set.hpp> // serialize ItemIdSet

namespace sf1r
{

class OrderManager
{
public:
    OrderManager(
        const std::string& path
    );

    void addOrder(
        sf1r::orderid_t orderId,
        std::list<sf1r::itemid_t>& items);

    void flush();
private:
    ItemIndex item_order_index_;
};

} // namespace sf1r

#endif // ORDER_MANAGER_H

