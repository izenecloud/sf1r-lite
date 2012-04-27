#include "RemoteItemManagerTestFixture.h"
#include <aggregator-manager/MasterServerConnector.h>
#include <recommend-manager/item/RemoteItemFactory.h>
#include <recommend-manager/item/ItemIdGenerator.h>
#include <recommend-manager/item/ItemManager.h>
#include <recommend-manager/item/ItemContainer.h>
#include <document-manager/Document.h>

#include <boost/test/unit_test.hpp>

namespace
{
const std::string SEARCH_MASTER_HOST = "localhost";
const int SEARCH_MASTER_PORT = 19004;
}

namespace sf1r
{

RemoteItemManagerTestFixture::RemoteItemManagerTestFixture()
    : addressFinderStub_(SEARCH_MASTER_HOST, SEARCH_MASTER_PORT)
{
    MasterServerConnector::get()->init(&addressFinderStub_);
}

void RemoteItemManagerTestFixture::resetInstance()
{
    BOOST_TEST_MESSAGE("create ItemManager");

    RemoteItemFactory itemFactory(collectionName_);
    itemManager_.reset(itemFactory.createItemManager());
    itemIdGenerator_.reset(itemFactory.createItemIdGenerator());
}

void RemoteItemManagerTestFixture::startSearchMasterServer()
{
    createSearchService_();
    createCollectionHandler_();

    masterServer_.reset(new MasterServer);
    masterServer_->start(SEARCH_MASTER_HOST, SEARCH_MASTER_PORT);
}

void RemoteItemManagerTestFixture::createCollectionHandler_()
{
    CollectionHandler* collectionHandler = new CollectionHandler(collectionName_);
    collectionHandler->registerService(indexSearchService_.get());

    CollectionManager* collectionManager = CollectionManager::get();
    collectionManager->collectionHandlers_[collectionName_] = collectionHandler;
}

void RemoteItemManagerTestFixture::checkGetItemFail()
{
    BOOST_TEST_MESSAGE("check " << itemMap_.size() << " items, expect fail");

    Document doc;
    SingleItemContainer itemContainer(doc);
    for (itemid_t i=1; i <= ItemManagerTestFixture::maxItemId_; ++i)
    {
        doc.setId(i);
        BOOST_CHECK(itemManager_->getItemProps(propList_, itemContainer) == false);
        BOOST_CHECK(doc.isEmpty());
        BOOST_CHECK(itemManager_->hasItem(i) == false);
    }

    itemid_t nonExistId = ItemManagerTestFixture::maxItemId_ + 1;
    doc.setId(nonExistId);
    BOOST_CHECK(itemManager_->getItemProps(propList_, itemContainer) == false);
    BOOST_CHECK(doc.isEmpty());
    BOOST_CHECK(itemManager_->hasItem(nonExistId) == false);
}

} // namespace sf1r
