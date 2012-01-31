#include "PurchaseManagerTestFixture.h"
#include "test_util.h"
#include <recommend-manager/storage/PurchaseManager.h>

#include <boost/test/unit_test.hpp>

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
    split_str_to_items(items, orderItemVec);

    BOOST_CHECK(purchaseManager_->addPurchaseItem(userId, orderItemVec, NULL));

    std::set<itemid_t>& itemIdSet = purchaseMap_[userId];
    for (unsigned int i = 0; i < orderItemVec.size(); ++i)
    {
        itemIdSet.insert(orderItemVec[i]);
    }
}

void PurchaseManagerTestFixture::addRandItem(const std::string& userId, int itemCount)
{
    addPurchaseItem(userId, generate_rand_items_str(itemCount));
}

void PurchaseManagerTestFixture::checkPurchaseManager() const
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
