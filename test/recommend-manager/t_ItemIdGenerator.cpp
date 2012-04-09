/// @file t_ItemIdGenerator.cpp
/// @brief test item id conversions in each ItemIdGenerator implementations
/// @author Jun Jiang
/// @date Created 2012-02-14
///

#include "ItemIdGeneratorTestFixture.h"
#include "RemoteItemIdGeneratorTestFixture.h"
#include "test_util.h"
#include <recommend-manager/item/ItemIdGenerator.h>
#include <recommend-manager/item/LocalItemIdGenerator.h>
#include <recommend-manager/item/SingleCollectionItemIdGenerator.h>
#include <recommend-manager/item/RemoteItemIdGenerator.h>
#include <log-manager/LogServerConnection.h>
#include <common/Utilities.h>
#include <util/ustring/UString.h>

#include <boost/test/unit_test.hpp>
#include <boost/lexical_cast.hpp>
#include <string>

using namespace std;
using namespace sf1r;

namespace
{
const izenelib::util::UString::EncodingType ENCODING_UTF8 = izenelib::util::UString::UTF_8;

const string TEST_DIR = "recommend_test/t_ItemIdGenerator";

const string LOCAL_DIR = TEST_DIR + "/local";
const string LOCAL_ID_MANAGER_PREFIX = LOCAL_DIR + "/item";

const string SINGLE_COLLECTION_DIR = TEST_DIR + "/single";
const string LOG_SERVER_DIR = TEST_DIR + "/log_server";

const string REMOTE_COLLECTION_1 = "example_1";
const string REMOTE_COLLECTION_2 = "example_2";

void checkItemId(ItemIdGeneratorTestFixture& fixture, itemid_t maxItemId)
{
    fixture.checkNonExistId();

    fixture.checkStrToItemId(maxItemId);
    fixture.checkItemIdToStr(maxItemId);

    fixture.checkNonExistId();
}

void checkGeneratorError(ItemIdGenerator& generator)
{
    itemid_t itemId = 1;
    std::string str = boost::lexical_cast<std::string>(itemId);

    BOOST_CHECK(generator.strIdToItemId(str, itemId) == false);
    BOOST_CHECK(generator.itemIdToStrId(itemId, str) == false);
    BOOST_CHECK_EQUAL(generator.maxItemId(), 0U);
}

}

BOOST_AUTO_TEST_SUITE(ItemIdGeneratorTest)

BOOST_FIXTURE_TEST_CASE(checkLocal, ItemIdGeneratorTestFixture)
{
    create_empty_directory(LOCAL_DIR);

    LocalItemIdGenerator::IDManagerType idManager(LOCAL_ID_MANAGER_PREFIX);
    itemIdGenerator_.reset(new LocalItemIdGenerator(idManager));

    BOOST_TEST_MESSAGE("check empty...");
    checkNonExistId();

    BOOST_TEST_MESSAGE("insert into IDManager...");
    itemid_t maxItemId = 10;
    for (itemid_t goldId=1; goldId<=maxItemId; ++goldId)
    {
        itemid_t id = 0;
        std::string str = boost::lexical_cast<std::string>(goldId);

        BOOST_CHECK(idManager.getDocIdByDocName(Utilities::md5ToUint128(str), id) == false);
        BOOST_CHECK_EQUAL(id, goldId);
    }

    BOOST_TEST_MESSAGE("check inserted id...");
    checkStrToItemId(maxItemId);

    // convert from id to string is not supported
    itemid_t itemId = 1;
    string strId;
    BOOST_CHECK(itemIdGenerator_->itemIdToStrId(itemId, strId) == false);

    checkNonExistId();
}

BOOST_FIXTURE_TEST_CASE(checkSingleCollection, ItemIdGeneratorTestFixture)
{
    string dir = SINGLE_COLLECTION_DIR;
    create_empty_directory(dir);

    itemIdGenerator_.reset(new SingleCollectionItemIdGenerator(dir));

    itemid_t maxItemId = 10;

    BOOST_TEST_MESSAGE("insert id 1st time...");
    checkItemId(*this, maxItemId);

    // flush first
    itemIdGenerator_.reset();
    itemIdGenerator_.reset(new SingleCollectionItemIdGenerator(SINGLE_COLLECTION_DIR));

    maxItemId *= 2;
    BOOST_TEST_MESSAGE("insert id 2nd time...");
    checkItemId(*this, maxItemId);
}

BOOST_FIXTURE_TEST_CASE(checkSingleCollectionMultiThread, ItemIdGeneratorTestFixture)
{
    string dir = SINGLE_COLLECTION_DIR;
    create_empty_directory(dir);

    itemIdGenerator_.reset(new SingleCollectionItemIdGenerator(dir));

    int threadNum = 5;
    itemid_t maxItemId = 1000;

    checkMultiThread(threadNum, maxItemId);
}

BOOST_FIXTURE_TEST_CASE(checkRemote, RemoteItemIdGeneratorTestFixture)
{
    string dir = LOG_SERVER_DIR;
    create_empty_directory(dir);

    BOOST_REQUIRE(startRpcLogServer(dir));

    itemid_t maxItemId = 10;

    BOOST_TEST_MESSAGE("check " << REMOTE_COLLECTION_1 << "...");
    itemIdGenerator_.reset(new RemoteItemIdGenerator(REMOTE_COLLECTION_1));
    checkItemId(*this, maxItemId);

    maxItemId *= 2;
    maxItemId_ = 0; // reset maxItemId_ for new collection
    BOOST_TEST_MESSAGE("check " << REMOTE_COLLECTION_2 << "...");
    itemIdGenerator_.reset(new RemoteItemIdGenerator(REMOTE_COLLECTION_2));
    checkItemId(*this, maxItemId);
}

BOOST_FIXTURE_TEST_CASE(checkRemoteMultiThread, RemoteItemIdGeneratorTestFixture)
{
    string dir = LOG_SERVER_DIR;
    create_empty_directory(dir);

    BOOST_REQUIRE(startRpcLogServer(dir));

    itemIdGenerator_.reset(new RemoteItemIdGenerator(REMOTE_COLLECTION_1));

    int threadNum = 5;
    itemid_t maxItemId = 1000;

    checkMultiThread(threadNum, maxItemId);
}

BOOST_FIXTURE_TEST_CASE(checkRemoteNotConnect, RemoteItemIdGeneratorTestFixture)
{
    string dir = LOG_SERVER_DIR;
    create_empty_directory(dir);

    // to test connection failed, below line is commented out intentionally
    //BOOST_REQUIRE(startRpcLogServer(dir));

    itemIdGenerator_.reset(new RemoteItemIdGenerator(REMOTE_COLLECTION_1));

    checkGeneratorError(*itemIdGenerator_);
}

BOOST_FIXTURE_TEST_CASE(checkRemoteHostNotResolve, RemoteItemIdGeneratorTestFixture)
{
    string dir = LOG_SERVER_DIR;
    create_empty_directory(dir);

    LogServerConnectionConfig newConfig;
    newConfig.host.clear();
    connection_.init(newConfig);

    BOOST_REQUIRE(startRpcLogServer(dir));

    itemIdGenerator_.reset(new RemoteItemIdGenerator(REMOTE_COLLECTION_1));

    checkGeneratorError(*itemIdGenerator_);
}

BOOST_AUTO_TEST_SUITE_END()
