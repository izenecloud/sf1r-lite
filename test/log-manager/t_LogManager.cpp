/**
 * @file t_LogManager.cpp
 * @brief A test unit for LogManager class.\n
 * @author Jinli Liu
 * @date 2008-08-11
 * @date update 2010-05-13 Jia Guo for the new LogManager.
 * @details
 *
 * =====================  TEST SCHEME  =========================\n
 * Test interfaces of LogManager class.
 * -# check saving query log, insert 10000 query log string into storage(SequentialDB)
 * -# check saving collection query log, insert 10000 collection query log string into storage(SequentialDB)
 * -# check saving click count of each document, increase click count of each 1000 document to 10000
 * -# check saving result docId list, insert 10000 document id list for each 10000 query log string into storage(SequentialDB)
 * -# generate popular keyword cluster with query log data which is created in step1 and get related keyword list from cluster data
 */
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <log-manager/LogManager.h>



#include "def.hpp"
using namespace sf1r;


BOOST_AUTO_TEST_SUITE(LogManager_test)

BOOST_AUTO_TEST_CASE(LogManager_common_test)
{
//    boost::filesystem::remove_all("./test_log_manager_dir");
//    LogManager logManager("test", "./test_log_manager_dir/");
//    LogManagerTestDef def(&logManager);
//    boost::posix_time::ptime now = boost::posix_time::second_clock::local_time();
//    def.insertQuery(izenelib::util::UString("aaaa", izenelib::util::UString::UTF_8), now);
//    def.insertQuery(izenelib::util::UString("ssss", izenelib::util::UString::UTF_8), now);
//    def.insertQuery(izenelib::util::UString("aawwwaa", izenelib::util::UString::UTF_8), now);
//    def.insertQuery(izenelib::util::UString("ssssaaaa", izenelib::util::UString::UTF_8), now);
//    def.insertQuery(izenelib::util::UString("retert", izenelib::util::UString::UTF_8), now);
//    def.insertQuery(izenelib::util::UString("dddddd", izenelib::util::UString::UTF_8), now);
//    def.insertQuery(izenelib::util::UString("aawerwrweraa", izenelib::util::UString::UTF_8), now);
//    BOOST_CHECK( def.isSucc() == true );
//    boost::filesystem::remove_all("./test_log_manager_dir");
}

BOOST_AUTO_TEST_SUITE_END()


