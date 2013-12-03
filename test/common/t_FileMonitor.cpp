/**
 * @file t_FileMonitor.cpp
 * @brief test FileMonitor
 */

#include "FileMonitorTestFixture.h"
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <fstream>
#include <iostream>
#include <sstream>
#include <ctime>
#include <cstdlib> // system
#include <unistd.h> // sleep
#include <sys/inotify.h>
#include <glog/logging.h>

using namespace sf1r;
namespace bfs = boost::filesystem;

BOOST_FIXTURE_TEST_SUITE(FileMonitorTest, FileMonitorTestFixture)

BOOST_AUTO_TEST_CASE(testUpdateTimeStamp)
{
    const std::string watchName("a.txt");
    const uint32_t event = IN_ATTRIB;

    // create file
    const std::string path = dirName_ + "/" + watchName;
    std::ofstream ofs(path.c_str());
    ofs.close();

    BOOST_CHECK(monitor_.addWatch(path, event));
    start();

    BOOST_CHECK_EQUAL(actualFileName_, "");
    BOOST_CHECK_EQUAL(actualMask_, 0);

    // update file timestamp
    bfs::last_write_time(path, time(NULL));

    // wait for event is triggered and processed
    sleep(1);

    // for event IN_ATTRIB, it's found that the file name got is empty
    BOOST_CHECK_EQUAL(actualFileName_, "");
    BOOST_CHECK(actualMask_ & event);
}

BOOST_AUTO_TEST_CASE(testModifyFile)
{
    const std::string watchName("a.txt");
    const uint32_t event = IN_MODIFY;

    // create file
    const std::string path = dirName_ + "/" + watchName;
    {
        std::ofstream ofs(path.c_str());
        ofs << "hello" << std::endl;
        ofs.close();
    }

    BOOST_CHECK(monitor_.addWatch(path, event));
    start();

    BOOST_CHECK_EQUAL(actualFileName_, "");
    BOOST_CHECK_EQUAL(actualMask_, 0);

    // modify file content
    {
        std::ofstream ofs(path.c_str());
        ofs << "world" << std::endl;
        ofs.close();
    }

    // wait for event is triggered and processed
    sleep(1);

    // for event IN_MODIFY, it's found that the file name got is empty
    BOOST_CHECK_EQUAL(actualFileName_, "");
    BOOST_CHECK(actualMask_ & event);
}

BOOST_AUTO_TEST_CASE(testCreateFile)
{
    const uint32_t event = IN_CREATE;
    const std::string path = dirName_;

    BOOST_CHECK(monitor_.addWatch(path, event));
    start();

    BOOST_CHECK_EQUAL(actualFileName_, "");
    BOOST_CHECK_EQUAL(actualMask_, 0);

    // create file
    const std::string fileName = "a.txt";
    const std::string filePath = dirName_ + "/" + fileName;
    {
        std::ofstream ofs(filePath.c_str());
        ofs << "hello" << std::endl;
        ofs.close();
    }

    // wait for event is triggered and processed
    sleep(1);

    BOOST_CHECK_EQUAL(actualFileName_, fileName);
    BOOST_CHECK(actualMask_ & event);
}

BOOST_AUTO_TEST_CASE(testNotExistFile)
{
    const std::string watchName("a.txt");
    const uint32_t event = IN_ATTRIB;
    const std::string path = dirName_ + "/" + watchName;

    BOOST_CHECK(monitor_.addWatch(path, event) == false);
    start();

    BOOST_CHECK_EQUAL(actualFileName_, "");
    BOOST_CHECK_EQUAL(actualMask_, 0);

    // create file
    std::ofstream ofs(path.c_str());
    ofs << "hello" << std::endl;
    ofs.close();

    // wait for event is triggered and processed
    sleep(1);

    // as no event is triggered, the result should not be changed
    BOOST_CHECK_EQUAL(actualFileName_, "");
    BOOST_CHECK_EQUAL(actualMask_, 0);
}

BOOST_AUTO_TEST_CASE(testRSyncDir)
{
    const std::string dirA = dirName_ + "/a/";
    const std::string dirB = dirName_ + "/b/";
    bfs::create_directory(dirA);
    bfs::create_directory(dirB);

    const std::string fileName1 = "1.txt";
    const std::string fileName2 = "2.txt";
    const std::string lastFileName = "z.update.time";

    std::ostringstream oss;
    oss << "touch " << dirA << fileName1
        << " && touch " << dirA << fileName2
        << " && touch " << dirA << lastFileName
        << " && rsync -azvP --delete " << dirA << " " << dirB;
    const std::string command = oss.str();

    LOG(INFO) << "create files: " << command;
    BOOST_REQUIRE_EQUAL(std::system(command.c_str()), 0);

    const std::string watchPath = dirB;
    const uint32_t event = IN_MOVED_TO;

    LOG(INFO) << "watch path: " << watchPath;
    BOOST_CHECK(monitor_.addWatch(watchPath, event));
    start();

    // wait for watch is added
    sleep(1);

    checkCommand(command, lastFileName, event);

    resetStatus();

    checkCommand(command, lastFileName, event);
}

BOOST_AUTO_TEST_SUITE_END()
