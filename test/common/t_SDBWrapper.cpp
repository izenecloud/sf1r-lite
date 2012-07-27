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

template <typename KeyT, typename ValueT>
struct EachPairFunc
{
    typedef std::pair<KeyT, ValueT> PairT;

    std::vector<PairT> array_;

    void operator()(const KeyT& key, const ValueT& value)
    {
        array_.push_back(PairT(key, value));
    }
};

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

        std::string keyStr2("key_002");
        unsigned int value1 = 2;
        unsigned int value2 = 4;
        BOOST_CHECK(db.update(keyStr2, value2));

        EachPairFunc<std::string, unsigned int> func;
        BOOST_CHECK(db.forEach(func));

        BOOST_REQUIRE_EQUAL(func.array_.size(), 2U);
        BOOST_CHECK_EQUAL(func.array_[0].first, keyStr);
        BOOST_CHECK_EQUAL(func.array_[0].second, value1);
        BOOST_CHECK_EQUAL(func.array_[1].first, keyStr2);
        BOOST_CHECK_EQUAL(func.array_[1].second, value2);
    }
}

BOOST_AUTO_TEST_SUITE_END()
