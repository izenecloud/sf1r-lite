#include "VisitManagerTestFixture.h"
#include <recommend-manager/VisitManager.h>
#include <recommend-manager/RecommendMatrix.h>

#include <boost/test/unit_test.hpp>
#include <cstdlib> // rand()

namespace
{

template<class CollectionType>
void checkCollection(const CollectionType& collection1, const CollectionType& collection2)
{
    BOOST_CHECK_EQUAL_COLLECTIONS(collection1.begin(), collection1.end(),
                                  collection2.begin(), collection2.end());
}

}

namespace sf1r
{

VisitManagerTestFixture::VisitManagerTestFixture()
    : visitManager_(NULL)
{
}

void VisitManagerTestFixture::setVisitManager(VisitManager* visitManager)
{
    visitManager_ = visitManager;
}

void VisitManagerTestFixture::addVisitItem(
    const std::string& sessionId,
    const std::string& userId,
    itemid_t itemId,
    bool isRecItem,
    RecommendMatrix* matrix
)
{
    UserRecord& record = visitMap_[userId];
    record.visitItems.insert(itemId);

    if (isRecItem)
    {
        record.recommendItems.insert(itemId);
    }

    if (record.sessionId != sessionId)
    {
        record.sessionId = sessionId;
        record.sessionItems.clear();
    }
    record.sessionItems.insert(itemId);

    BOOST_CHECK(visitManager_->addVisitItem(sessionId, userId, itemId, isRecItem, matrix));
}

void VisitManagerTestFixture::addRandItem(
    const std::string& sessionId,
    const std::string& userId,
    int itemCount
)
{
    for (int i = 0; i < itemCount; ++i)
    {
        itemid_t itemId = std::rand();
        bool isRecItem = std::rand() & 1; // odd to recommend, even not to recommend
        addVisitItem(sessionId, userId, itemId, isRecItem, NULL);
    }
}

void VisitManagerTestFixture::checkVisitManager() const
{
    for (UserVisitMap::const_iterator it = visitMap_.begin();
        it != visitMap_.end(); ++it)
    {
        const std::string& userId = it->first;
        const UserRecord& record = it->second;

        ItemIdSet visitItems;
        BOOST_CHECK(visitManager_->getVisitItemSet(userId, visitItems));
        checkCollection(visitItems, record.visitItems);

        ItemIdSet recommendItems;
        BOOST_CHECK(visitManager_->getRecommendItemSet(userId, recommendItems));
        checkCollection(recommendItems, record.recommendItems);

        VisitSession visitSession;
        BOOST_CHECK(visitManager_->getVisitSession(userId, visitSession));
        BOOST_CHECK_EQUAL(visitSession.sessionId_, record.sessionId);
        checkCollection(visitSession.itemSet_, record.sessionItems);
    }
}

} // namespace sf1r
