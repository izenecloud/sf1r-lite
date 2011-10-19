///
/// @file t_IDGenerator.cpp
/// @brief test IDGenerator in generating id from string
/// @author Jun Jiang
/// @date Created 2011-04-25
///

#include <ir/id_manager/IDGenerator.h>

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/lexical_cast.hpp>

#include <string>
#include <map>
#include <set>

using namespace std;
using namespace boost;

namespace bfs = boost::filesystem;

namespace
{
const char* TEST_DIR_STR = "recommend_test/t_IDGenerator";
const char* ID_DB_NAME = "id";
}

BOOST_AUTO_TEST_SUITE(IDGeneratorTest)

BOOST_AUTO_TEST_CASE(test_UniqueIDGenerator)
{
    bfs::remove_all(TEST_DIR_STR);
    bfs::create_directories(TEST_DIR_STR);

    bfs::path idName(bfs::path(TEST_DIR_STR) / ID_DB_NAME);

    {
        izenelib::ir::idmanager::UniqueIDGenerator<std::string, uint32_t> idGenerator(idName.string());
        for (uint32_t i = 1; i < 100; ++i)
        {
            string str = boost::lexical_cast<string>(i);
            uint32_t id;
            // not yet converted before
            BOOST_CHECK(idGenerator.conv(str, id) == false);
            BOOST_CHECK_EQUAL(id, i);
        }
    }

    {
        izenelib::ir::idmanager::UniqueIDGenerator<std::string, uint32_t> idGenerator(idName.string());
        for (uint32_t i = 1; i < 100; ++i)
        {
            string str = boost::lexical_cast<string>(i);
            uint32_t id;
            // already converted
            BOOST_CHECK(idGenerator.conv(str, id) == true);
            BOOST_CHECK_EQUAL(id, i);
        }
    }

    {
        izenelib::ir::idmanager::UniqueIDGenerator<std::string, uint32_t> idGenerator(idName.string());
        for (uint32_t i = 100; i < 200; ++i)
        {
            string str = boost::lexical_cast<string>(i);
            uint32_t id;
            // not yet converted before
            BOOST_CHECK(idGenerator.conv(str, id) == false);
            BOOST_CHECK_EQUAL(id, i);
        }
    }
}


BOOST_AUTO_TEST_CASE(test_HashIDGenerator)
{
    map<string, uint32_t> idMap;
    set<uint32_t> idSet;

    {
        izenelib::ir::idmanager::HashIDGenerator<std::string, uint32_t> idGenerator("hash_id");
        for (uint32_t i = 1; i < 10000; ++i)
        {
            string str = boost::lexical_cast<string>(i);
            uint32_t id;
            // always false
            BOOST_CHECK(idGenerator.conv(str, id) == false);
            //BOOST_MESSAGE("hash from str " << str << " to int " << id);
            idMap[str] = id;
            pair<set<uint32_t>::iterator, bool> res = idSet.insert(id);
            // not generated before
            BOOST_CHECK(res.second);
        }
    }

    {
        izenelib::ir::idmanager::HashIDGenerator<std::string, uint32_t> idGenerator("hash_id");
        for (map<string, uint32_t>::const_iterator it = idMap.begin();
            it != idMap.end(); ++it)
        {
            uint32_t id;
            // always false
            BOOST_CHECK(idGenerator.conv(it->first, id) == false);
            BOOST_CHECK_EQUAL(id, it->second);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END() 
