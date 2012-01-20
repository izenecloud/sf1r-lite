#include "PurchaseManagerTestFixture.h"
#include <recommend-manager/storage/PurchaseManager.h>

#include <boost/test/unit_test.hpp>
#include <sstream>

namespace sf1r
{

PurchaseManagerTestFixture::PurchaseManagerTestFixture()
    : purchaseManager_(NULL)
{
}

void PurchaseManagerTestFixture::setPurchaseManager(PurchaseManager* purchaseManager)
{
    purchaseManager_ = purchaseManager;
}

void PurchaseManagerTestFixture::addPurchaseItem(const std::string& userId, const std::string& items)
{
    std::vector<itemid_t> orderItemVec;
    std::istringstream ss(items);
    itemid_t itemId;
    while (ss >> itemId)
    {
        orderItemVec.push_back(itemId);
    }

    BOOST_CHECK(purchaseManager_->addPurchaseItem(userId, orderItemVec, NULL));

    std::set<itemid_t>& itemIdSet = purchaseMap_[userId];
    for (unsigned int i = 0; i < orderItemVec.size(); ++i)
    {
        itemIdSet.insert(orderItemVec[i]);
    }
}

void PurchaseManagerTestFixture::checkPurchaseManager()
{
    for (PurchaseMap::const_iterator it = purchaseMap_.begin();
        it != purchaseMap_.end(); ++it)
    {
        ItemIdSet itemIdSet;
        BOOST_CHECK(purchaseManager_->getPurchaseItemSet(it->first, itemIdSet));
        BOOST_CHECK_EQUAL_COLLECTIONS(itemIdSet.begin(), itemIdSet.end(),
                                      it->second.begin(), it->second.end());
    }
}

} // namespace sf1r
