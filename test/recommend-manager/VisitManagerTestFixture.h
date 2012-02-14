///
/// @file VisitManagerTestFixture.h
/// @brief fixture class to test VisitManager
/// @author Jun Jiang
/// @date Created 2012-01-20
///

#ifndef VISIT_MANAGER_TEST_FIXTURE
#define VISIT_MANAGER_TEST_FIXTURE

#include "RecommendStorageTestFixture.h"
#include <recommend-manager/common/RecTypes.h>

#include <map>
#include <string>

namespace sf1r
{
class VisitManager;
class RecommendMatrix;

class VisitManagerTestFixture : public RecommendStorageTestFixture
{
public:
    virtual void resetInstance();

    void addVisitItem(
        const std::string& sessionId,
        const std::string& userId,
        itemid_t itemId,
        RecommendMatrix* matrix
    );

    void visitRecommendItem(
        const std::string& userId,
        itemid_t itemId
    );

    /**
     * add random items as user visited items.
     * @attention in calling VisitManager::addVisitItem(), @p matrix would be NULL,
     *            that is, no @p matrix would be updated in this function.
     */
    void addRandItem(
        const std::string& sessionId,
        const std::string& userId,
        int itemCount
    );

    void checkVisitManager() const;

protected:
    boost::scoped_ptr<VisitManager> visitManager_;

    struct UserRecord
    {
        ItemIdSet visitItems;
        ItemIdSet recommendItems;
        ItemIdSet sessionItems;
        std::string sessionId;
    };

    typedef std::map<std::string, UserRecord> UserVisitMap;
    UserVisitMap visitMap_;
};

} // namespace sf1r

#endif //VISIT_MANAGER_TEST_FIXTURE
