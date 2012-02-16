/// @file t_ItemIdGenerator.cpp
/// @brief test item id conversions in each ItemIdGenerator implementations
/// @author Jun Jiang
/// @date Created 2012-02-14
///

#include "ItemIdGeneratorTestFixture.h"
#include <recommend-manager/item/ItemIdGenerator.h>
#include <recommend-manager/item/LocalItemIdGenerator.h>
#include <recommend-manager/item/SingleCollectionItemIdGenerator.h>
#include <util/ustring/UString.h>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <string>

using namespace std;
using namespace sf1r;

namespace bfs = boost::filesystem;

namespace
{
const izenelib::util::UString::EncodingType ENCODING_UTF8 = izenelib::util::UString::UTF_8;

const string TEST_DIR = "recommend_test/t_ItemIdGenerator/";
const string LOCAL_DIR = TEST_DIR + "local";
const string SINGLE_COLLECTION_DIR = TEST_DIR + "single";
}

BOOST_AUTO_TEST_SUITE(ItemIdGeneratorTest)

BOOST_FIXTURE_TEST_CASE(checkLocal, ItemIdGeneratorTestFixture)
{
    string dir = LOCAL_DIR;
    bfs::remove_all(dir);
    bfs::create_directories(dir);

    string pathPrefix = (bfs::path(dir) / "item").string();
    LocalItemIdGenerator::IDManagerType idManager(pathPrefix);
    itemIdGenerator_.reset(new LocalItemIdGenerator(idManager));

    BOOST_TEST_MESSAGE("check empty...");
    itemid_t itemId = 1;
    string strId = boost::lexical_cast<std::string>(itemId);
    BOOST_CHECK(itemIdGenerator_->strIdToItemId(strId, itemId) == false);
    BOOST_CHECK(itemIdGenerator_->itemIdToStrId(itemId, strId) == false);
    BOOST_CHECK_EQUAL(itemIdGenerator_->maxItemId(), 0U);

    BOOST_TEST_MESSAGE("insert into IDManager...");
    itemid_t maxItemId = 10;
    for (itemid_t goldId=1; goldId<=maxItemId; ++goldId)
    {
        std::string str = boost::lexical_cast<std::string>(goldId);
        izenelib::util::UString ustr(str, ENCODING_UTF8);
        itemid_t id = 0;

        BOOST_CHECK(idManager.getDocIdByDocName(ustr, id) == false);
        BOOST_CHECK_EQUAL(id, goldId);
    }

    BOOST_TEST_MESSAGE("check inserted id...");
    checkStrToItemId(maxItemId);
    // convert from id to string is not supported
    BOOST_CHECK(itemIdGenerator_->itemIdToStrId(itemId, strId) == false);

    BOOST_TEST_MESSAGE("check non-existing id...");
    itemId = maxItemId + 1;
    strId = boost::lexical_cast<std::string>(itemId);
    BOOST_CHECK(itemIdGenerator_->strIdToItemId(strId, itemId) == false);
    BOOST_CHECK(itemIdGenerator_->itemIdToStrId(itemId, strId) == false);
    BOOST_CHECK_EQUAL(itemIdGenerator_->maxItemId(), maxItemId);
}

BOOST_FIXTURE_TEST_CASE(checkSingleCollection, ItemIdGeneratorTestFixture)
{
    string dir = SINGLE_COLLECTION_DIR;
    bfs::remove_all(dir);
    bfs::create_directories(dir);

    itemIdGenerator_.reset(new SingleCollectionItemIdGenerator(dir));

    BOOST_TEST_MESSAGE("insert id 1st time...");
    itemid_t maxItemId = 10;
    checkStrToItemId(maxItemId);
    checkItemIdToStr(maxItemId);

    // flush first
    itemIdGenerator_.reset();
    itemIdGenerator_.reset(new SingleCollectionItemIdGenerator(SINGLE_COLLECTION_DIR));

    BOOST_TEST_MESSAGE("insert id 2nd time...");
    maxItemId *= 2; 
    checkStrToItemId(maxItemId);
    checkItemIdToStr(maxItemId);
}

BOOST_FIXTURE_TEST_CASE(checkSingleCollectionMultiThread, ItemIdGeneratorTestFixture)
{
    string dir = SINGLE_COLLECTION_DIR;
    bfs::remove_all(dir);
    bfs::create_directories(dir);

    itemIdGenerator_.reset(new SingleCollectionItemIdGenerator(dir));

    int threadNum = 5;
    itemid_t maxItemId = 1000;

    checkMultiThread(threadNum, maxItemId);
}

BOOST_AUTO_TEST_SUITE_END() 
