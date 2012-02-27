///
/// @file SearchMasterItemManagerTestFixture.h
/// @brief fixture class to test SearchMasterItemManager
/// @author Jun Jiang
/// @date Created 2012-02-23
///

#ifndef SEARCH_MASTER_ITEM_MANAGER_TEST_FIXTURE
#define SEARCH_MASTER_ITEM_MANAGER_TEST_FIXTURE

#include "ItemManagerTestFixture.h"
#include "RemoteItemIdGeneratorTestFixture.h"
#include <index/IndexBundleConfiguration.h>

#include <ir/id_manager/IDManager.h>

namespace sf1r
{
class IndexSearchService;

class SearchMasterItemManagerTestFixture : public ItemManagerTestFixture,
                                           public RemoteItemIdGeneratorTestFixture
{
public:
    SearchMasterItemManagerTestFixture();
    virtual ~SearchMasterItemManagerTestFixture();

    virtual void resetInstance();

protected:
    void createSearchService_();
    void createIdManager_();

    virtual void insertItemId_(const std::string& strId, itemid_t goldId);

protected:
    typedef izenelib::ir::idmanager::IDManager IDManagerType;
    boost::shared_ptr<IDManagerType> idManager_;

    const std::string collectionName_;
    IndexBundleConfiguration indexBundleConfig_;
    boost::scoped_ptr<IndexSearchService> indexSearchService_;
};

} // namespace sf1r

#endif // SEARCH_MASTER_ITEM_MANAGER_TEST_FIXTURE
