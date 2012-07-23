#include <LogServerStorage.h>
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

using namespace sf1r;

BOOST_AUTO_TEST_SUITE(LogHistoryDB_test)

BOOST_AUTO_TEST_CASE(LogHistoryDB_common_test)
{
    boost::filesystem::remove_all("./log_server_storage");
    // Parse config first
    LogServerCfg::get()->parse("../bin/config/logserver.cfg");

    BOOST_CHECK(LogServerStorage::get()->init());

    std::string ret_result;
    bool ret;
    uint128_t  testkey;
    testkey = 1;
    {
        boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->historyDBMutex());
        ret = LogServerStorage::get()->historyDB()->insert_olduuid(testkey, "123");
        BOOST_CHECK(ret);
        ret = LogServerStorage::get()->historyDB()->get_olduuid(testkey, ret_result);
        BOOST_CHECK(ret);
        BOOST_CHECK_EQUAL(ret_result, "123");
    }
    {
        boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->historyDBMutex());
        ret = LogServerStorage::get()->historyDB()->insert_olddocid(testkey, "123");
        BOOST_CHECK(ret);
        ret = LogServerStorage::get()->historyDB()->get_olddocid(testkey, ret_result);
        BOOST_CHECK(ret);
        BOOST_CHECK_EQUAL(ret_result, "123");
    }
    // test insert the same value
    {
        boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->historyDBMutex());
        ret = LogServerStorage::get()->historyDB()->insert_olduuid(testkey, "123");
        BOOST_CHECK(!ret);
        ret = LogServerStorage::get()->historyDB()->get_olduuid(testkey, ret_result);
        BOOST_CHECK(ret);
        BOOST_CHECK_EQUAL(ret_result, "123");
    }
    {
        boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->historyDBMutex());
        ret = LogServerStorage::get()->historyDB()->insert_olddocid(testkey, "123");
        BOOST_CHECK(!ret);
        ret = LogServerStorage::get()->historyDB()->get_olddocid(testkey, ret_result);
        BOOST_CHECK(ret);
        BOOST_CHECK_EQUAL(ret_result, "123");
    }

    // test insert the new value
    {
        boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->historyDBMutex());
        LogServerStorage::get()->historyDB()->insert_olduuid(testkey, "456");
        ret = LogServerStorage::get()->historyDB()->get_olduuid(testkey, ret_result);
    }
    BOOST_CHECK(ret);
    BOOST_CHECK_EQUAL(ret_result, "123,456");
    {
        boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->historyDBMutex());
        LogServerStorage::get()->historyDB()->insert_olddocid(testkey, "456");
        ret = LogServerStorage::get()->historyDB()->get_olddocid(testkey, ret_result);
    }
    BOOST_CHECK(ret);
    BOOST_CHECK_EQUAL(ret_result, "123,456");
    {
        boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->historyDBMutex());
        LogServerStorage::get()->historyDB()->insert_olddocid(testkey, "789");
        LogServerStorage::get()->historyDB()->insert_olddocid(testkey, "999");
        LogServerStorage::get()->historyDB()->insert_olddocid(testkey, "888");
        LogServerStorage::get()->historyDB()->insert_olddocid(testkey, "777");
    }
    // test remove value in head
    {
        boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->historyDBMutex());
        LogServerStorage::get()->historyDB()->remove_olddocid(testkey, "123");
        ret = LogServerStorage::get()->historyDB()->get_olddocid(testkey, ret_result);
    }
    BOOST_CHECK(ret);
    BOOST_CHECK_EQUAL(ret_result, "456,789,999,888,777");

    // test remove value in tail
    {
        boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->historyDBMutex());
        LogServerStorage::get()->historyDB()->remove_olddocid(testkey, "777");
        ret = LogServerStorage::get()->historyDB()->get_olddocid(testkey, ret_result);
    }
    BOOST_CHECK(ret);
    BOOST_CHECK_EQUAL(ret_result, "456,789,999,888");
    // test remove value in middle
    {
        boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->historyDBMutex());
        LogServerStorage::get()->historyDB()->remove_olddocid(testkey, "999");
        ret = LogServerStorage::get()->historyDB()->get_olddocid(testkey, ret_result);
    }
    BOOST_CHECK(ret);
    BOOST_CHECK_EQUAL(ret_result, "456,789,888");

    {
        boost::lock_guard<boost::mutex> lock(LogServerStorage::get()->historyDBMutex());
        LogServerStorage::get()->historyDB()->remove_olddocid(testkey, "456");
        LogServerStorage::get()->historyDB()->remove_olddocid(testkey, "789");
        LogServerStorage::get()->historyDB()->remove_olddocid(testkey, "888");
        ret = LogServerStorage::get()->historyDB()->get_olddocid(testkey, ret_result);
    }
    BOOST_CHECK(ret);
    BOOST_CHECK_EQUAL(ret_result, "");
}

BOOST_AUTO_TEST_SUITE_END()


