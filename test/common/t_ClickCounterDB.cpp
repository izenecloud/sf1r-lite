///
/// @file t_ClickCounterDB.cpp
/// @brief test counting clicks frequency
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2012-03-16
///

#include "ClickCounterDBTestFixture.h"
#include <util/ustring/UString.h>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

using namespace sf1r;

namespace bfs = boost::filesystem;

namespace
{
const char* TEST_DIR_STR = "click_counter_db_test";
const char* DB_FILE_NAME = "click_counter.db";
}

BOOST_AUTO_TEST_SUITE(ClickCounter_test)

BOOST_FIXTURE_TEST_CASE(testClickCount, ClickCounterDBTestFixture)
{
    bfs::path dir(TEST_DIR_STR);
    bfs::remove_all(dir);
    bfs::create_directory(dir);

    const std::string dbPath = (dir / DB_FILE_NAME).string();

    setTestData(100, 100); // key num, value num
    const int CLICK_NUM = 10000;

    resetInstance(dbPath);

    runClick(CLICK_NUM);
    checkCount();

    resetInstance(dbPath);
    checkCount();

    runClick(CLICK_NUM);
    checkCount();
}

BOOST_AUTO_TEST_CASE(testKeyTypeUString)
{
    bfs::path dir(TEST_DIR_STR);
    bfs::remove_all(dir);
    bfs::create_directory(dir);

    const std::string dbPath = (dir / DB_FILE_NAME).string();

    typedef ClickCounter<unsigned int, int> ClickCounterT;
    typedef SDBWrapper<izenelib::util::UString, ClickCounterT> DBType;
    ClickCounterT clickCounter;
    izenelib::util::UString keyStr("key_001", izenelib::util::UString::UTF_8);
    int value = 15;
    const int CLICK_NUM = 100;

    {
        DBType db(dbPath);

        BOOST_CHECK(db.get(keyStr, clickCounter));
        BOOST_CHECK_EQUAL(clickCounter.getTotalFreq(), 0);

        for (int i=0; i<CLICK_NUM; ++i)
        {
            clickCounter.click(value);
        }

        BOOST_CHECK(db.update(keyStr, clickCounter));
    }

    {
        DBType db(dbPath);

        BOOST_CHECK(db.get(keyStr, clickCounter));
        BOOST_CHECK_EQUAL(clickCounter.getTotalFreq(), CLICK_NUM);

        std::vector<ClickCounterT::value_type> values;
        std::vector<ClickCounterT::freq_type> freqs;
        clickCounter.getFreqClick(10, values, freqs);

        BOOST_CHECK_EQUAL(values.size(), 1);
        BOOST_CHECK_EQUAL(values[0], value);

        BOOST_CHECK_EQUAL(freqs.size(), 1);
        BOOST_CHECK_EQUAL(freqs[0], CLICK_NUM);
    }
}

BOOST_AUTO_TEST_SUITE_END() 
