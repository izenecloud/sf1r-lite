///
/// @file PurchaseManagerTestFixture.h
/// @brief fixture class to test PurchaseManager
/// @author Jun Jiang
/// @date Created 2012-01-20
///

#ifndef PURCHASE_MANAGER_TEST_FIXTURE
#define PURCHASE_MANAGER_TEST_FIXTURE

#include "RecommendStorageTestFixture.h"
#include <recommend-manager/RecTypes.h>

#include <map>
#include <string>
#include <set>

namespace sf1r
{
class PurchaseManager;

class PurchaseManagerTestFixture : public RecommendStorageTestFixture
{
public:
    virtual void resetInstance();

    void addPurchaseItem(const std::string& userId, const std::string& items);

    void addRandItem(const std::string& userId, int itemCount);

    void checkPurchaseManager() const;

protected:
    boost::scoped_ptr<PurchaseManager> purchaseManager_;

    typedef std::map<std::string, std::set<itemid_t> > PurchaseMap;
    PurchaseMap purchaseMap_;
};

} // namespace sf1r

#endif //PURCHASE_MANAGER_TEST_FIXTURE
