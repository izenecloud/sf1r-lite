///
/// @file t_SDBWrapper.cpp
/// @brief test SDBWrapper
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2012-06-20
///

#include <common/SDBWrapper.h>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

using namespace sf1r;

namespace bfs = boost::filesystem;

namespace
{
const char* TEST_DIR_STR = "sdb_wrapper_test";
const char* DB_FILE_NAME = "db.bin";
}

BOOST_AUTO_TEST_SUITE(SDBWrapper_test)

BOOST_AUTO_TEST_CASE(testSDBWrapper)
{
    bfs::path dir(TEST_DIR_STR);
    bfs::remove_all(dir);
    bfs::create_directory(dir);

    const std::string dbPath = (dir / DB_FILE_NAME).string();

    typedef SDBWrapper<std::string, unsigned int> SDBType;
    std::string keyStr("key_001");

    {
        SDBType db(dbPath);

        unsigned int value = 0;
        BOOST_CHECK(db.get(keyStr, value));
        BOOST_CHECK_EQUAL(value, 0U);

        value = 1;
        BOOST_CHECK(db.update(keyStr, value));
    }

    {
        SDBType db(dbPath);

        unsigned int value = 0;
        BOOST_CHECK(db.get(keyStr, value));
        BOOST_CHECK_EQUAL(value, 1U);

        value = 2;
        BOOST_CHECK(db.update(keyStr, value));

        BOOST_CHECK(db.get(keyStr, value));
        BOOST_CHECK_EQUAL(value, 2U);

        db.flush();
    }

    {
        SDBType db(dbPath);

        unsigned int value = 0;
        BOOST_CHECK(db.get(keyStr, value));
        BOOST_CHECK_EQUAL(value, 2U);
    }
}

BOOST_AUTO_TEST_SUITE_END()
