///
/// @file t_UserIdGenerator.cpp
/// @brief test UserIdGenerator in generating id from string
/// @author Jun Jiang
/// @date Created 2011-12-28
///

#include <recommend-manager/UserIdGenerator.h>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/lexical_cast.hpp>

#include <string>

using namespace std;
using namespace boost;
using namespace sf1r;

namespace bfs = boost::filesystem;

namespace
{
const char* TEST_DIR_STR = "recommend_test/t_UserIdGenerator";
const char* ID_DB_NAME = "id";
}

BOOST_AUTO_TEST_SUITE(UserIdGeneratorTest)

BOOST_AUTO_TEST_CASE(test_insert)
{
    bfs::remove_all(TEST_DIR_STR);
    bfs::create_directories(TEST_DIR_STR);

    bfs::path idName(bfs::path(TEST_DIR_STR) / ID_DB_NAME);

    {
        UserIdGenerator idGenerator(idName.string());
        for (uint32_t i = 1; i < 10; ++i)
        {
            string str = boost::lexical_cast<string>(i);
            uint32_t id;
            // not exist
            BOOST_CHECK(idGenerator.get(str, id) == false);
        }
    }

    {
        UserIdGenerator idGenerator(idName.string());
        for (uint32_t i = 1; i < 10; ++i)
        {
            string str = boost::lexical_cast<string>(i);
            uint32_t id = idGenerator.insert(str);
            BOOST_CHECK_EQUAL(id, i);
        }
    }

    {
        UserIdGenerator idGenerator(idName.string());
        for (uint32_t i = 1; i < 10; ++i)
        {
            string str = boost::lexical_cast<string>(i);
            uint32_t id;
            // exist
            BOOST_CHECK(idGenerator.get(str, id));
            BOOST_CHECK_EQUAL(id, i);
        }

        for (uint32_t i = 10; i < 20; ++i)
        {
            string str = boost::lexical_cast<string>(i);
            uint32_t id;
            // not exist
            BOOST_CHECK(idGenerator.get(str, id) == false);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END() 
