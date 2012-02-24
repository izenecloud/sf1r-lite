#include "SearchMasterItemManagerTestFixture.h"
#include <recommend-manager/item/ItemIdGenerator.h>
#include <recommend-manager/item/ItemManager.h>
#include <index/IndexSearchService.h>
#include <recommend-manager/item/SearchMasterItemFactory.h>
#include <aggregator-manager/SearchWorker.h>

#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>

namespace bfs = boost::filesystem;

namespace
{
const izenelib::util::UString::EncodingType ENCODING_TYPE = izenelib::util::UString::UTF_8;
const string COLLECTION_NAME = "example";
}

namespace sf1r
{

SearchMasterItemManagerTestFixture::SearchMasterItemManagerTestFixture()
    : indexBundleConfig_(COLLECTION_NAME)
{
    indexBundleConfig_.isSupportByAggregator_ = false;
    indexBundleConfig_.masterSearchCacheNum_ = 1000;
    indexBundleConfig_.searchCacheNum_ = 1000;
}

SearchMasterItemManagerTestFixture::~SearchMasterItemManagerTestFixture()
{
}

void SearchMasterItemManagerTestFixture::setTestDir(const std::string& dir)
{
    ItemManagerTestFixture::setTestDir(dir);

    bfs::path idDir(testDir_);
    idDir /= "id_manager";
    bfs::create_directory(idDir);

    idPathPrefix_  = (idDir / "item").string();
}

void SearchMasterItemManagerTestFixture::resetInstance()
{
    BOOST_TEST_MESSAGE("create ItemManager");

    idManager_.reset(new IDManagerType(idPathPrefix_));
    documentManager_.reset(new DocumentManager(dmPath_, schema_, ENCODING_TYPE, 2000));

    SearchWorker* searchWorker = new SearchWorker(&indexBundleConfig_);
    searchWorker->idManager_ = idManager_;
    searchWorker->documentManager_ = documentManager_;

    indexSearchService_.reset(new IndexSearchService(&indexBundleConfig_));
    indexSearchService_->searchWorker_.reset(searchWorker);

    SearchMasterItemFactory itemFactory(COLLECTION_NAME, *indexSearchService_);
    itemManager_.reset(itemFactory.createItemManager());
    itemIdGenerator_.reset(itemFactory.createItemIdGenerator());
}

void SearchMasterItemManagerTestFixture::insertItemId_(const std::string& strId, itemid_t goldId)
{
    izenelib::util::UString ustr(strId, ENCODING_TYPE);
    itemid_t id = 0;

    idManager_->getDocIdByDocName(ustr, id);
    BOOST_CHECK_EQUAL(id, goldId);

    BOOST_CHECK(itemIdGenerator_->strIdToItemId(strId, id));
    BOOST_CHECK_EQUAL(id, goldId);
}

} // namespace sf1r
