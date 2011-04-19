#include "PurchaseManager.h"

namespace sf1r
{

PurchaseManager::PurchaseManager(const std::string& path)
    : container_(path)
{
    container_.open();
}

void PurchaseManager::flush()
{
    container_.flush();
}

bool PurchaseManager::addPurchaseItem(
    userid_t userId,
    itemid_t itemId,
    double price,
    int quantity,
    const std::string& orderIdStr
)
{
    Purchase purchase;
    container_.getValue(userId, purchase);

    Order* order = NULL;
    bool isFind = false;
    if (!orderIdStr.empty())
    {
        for (OrderVec::reverse_iterator rit = purchase.orderVec_.rbegin();
            rit != purchase.orderVec_.rend(); ++rit)
        {
            if (!rit->orderIdStr_.empty() && rit->orderIdStr_ == orderIdStr)
            {
                order = &*rit;
                isFind = true;
                break;
            }
        }
    }

    if (!isFind)
    {
        purchase.orderVec_.push_back(Order());
        order = &purchase.orderVec_.back();
        order->orderIdStr_ = orderIdStr;
    }

    order->orderItemVec_.push_back(OrderItem());
    OrderItem& orderItem = order->orderItemVec_.back();
    orderItem.itemId_ = itemId;
    orderItem.price_ = price;
    orderItem.quantity_ = quantity;

    purchase.itemIdSet_.insert(itemId);

    return container_.update(userId, purchase);
}

bool PurchaseManager::getPurchaseItemSet(userid_t userId, ItemIdSet& itemIdSet)
{
    Purchase purchase;
    if (container_.getValue(userId, purchase))
    {
        itemIdSet = purchase.itemIdSet_;
        return true;
    }

    return false;
}

bool PurchaseManager::getPurchaseOrderVec(userid_t userId, OrderVec& orderVec)
{
    Purchase purchase;
    if (container_.getValue(userId, purchase))
    {
        orderVec = purchase.orderVec_;
        return true;
    }

    return false;
}

unsigned int PurchaseManager::purchaseUserNum()
{
    return container_.numItems();
}

PurchaseManager::SDBIterator PurchaseManager::begin()
{
    return SDBIterator(container_);
}

PurchaseManager::SDBIterator PurchaseManager::end()
{
    return SDBIterator();
}

} // namespace sf1r
