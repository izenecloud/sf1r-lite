///
/// @file RemoteItemManagerTestFixture.h
/// @brief fixture class to test RemoteItemManager
/// @author Jun Jiang
/// @date Created 2012-02-27
///

#ifndef REMOTE_ITEM_MANAGER_TEST_FIXTURE
#define REMOTE_ITEM_MANAGER_TEST_FIXTURE

#include "SearchMasterItemManagerTestFixture.h"
#include "SearchMasterAddressFinderStub.h"
#include <distribute/MasterServer.h>

#include <boost/scoped_ptr.hpp>

namespace sf1r
{

class RemoteItemManagerTestFixture : public SearchMasterItemManagerTestFixture
{
public:
    RemoteItemManagerTestFixture();

    virtual void resetInstance();

    void startSearchMasterServer();

    void checkGetItemFail();

protected:
    void createCollectionHandler_();

protected:
    SearchMasterAddressFinderStub addressFinderStub_;
    boost::scoped_ptr<MasterServer> masterServer_;
};

} // namespace sf1r

#endif // REMOTE_ITEM_MANAGER_TEST_FIXTURE
