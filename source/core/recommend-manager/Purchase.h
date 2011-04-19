/**
 * @file Purchase.h
 * @author Jun Jiang
 * @date 2011-04-19
 */

#ifndef PURCHASE_H
#define PURCHASE_H

#include "RecTypes.h"

#include <string>
#include <vector>

#include <boost/serialization/set.hpp> // serialize ItemIdSet
#include <boost/serialization/vector.hpp> // serialize OrderVec

namespace sf1r
{

struct OrderItem
{
    itemid_t itemId_;
    double price_;
    int quantity_;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & itemId_;
        ar & price_;
        ar & quantity_;
    }
};
typedef std::vector<OrderItem> OrderItemVec;

struct Order
{
    std::string orderIdStr_;
    OrderItemVec orderItemVec_;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & orderIdStr_;
        ar & orderItemVec_;
    }
};
typedef std::vector<Order> OrderVec;

struct Purchase
{
    ItemIdSet itemIdSet_;
    OrderVec orderVec_;

    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar & itemIdSet_;
        ar & orderVec_;
    }
};

} // namespace sf1r

#endif // PURCHASE_H
