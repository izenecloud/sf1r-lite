/**
 * @file t_FileMonitor.cpp
 * @brief test FileMonitor
 */

#include "FileMonitorTestFixture.h"
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <fstream>
#include <iostream>
#include <ctime>
#include <unistd.h> // sleep

using namespace sf1r;
namespace bfs = boost::filesystem;

BOOST_AUTO_TEST_SUITE(FileMonitorTest)

BOOST_AUTO_TEST_CASE(testUpdateTimeStamp)
{
    {
        FileMonitorTestFixture fixture;
        const std::string watchName("a.txt");
        const uint32_t event = IN_ATTRIB;

        // create file
        const std::string path = fixture.dirName_ + "/" + watchName;
        std::ofstream ofs(path.c_str());
        ofs.close();

        fixture.addWatch(path, event);
        fixture.monitor();

        BOOST_CHECK_EQUAL(fixture.actualFileName_, "");
        BOOST_CHECK_EQUAL(fixture.actualMask_, 0);

        bfs::last_write_time(path, time(NULL));
        // wait for event is triggered and processed
        sleep(1);

        // empty file name for event IN_ATTRIB
        BOOST_CHECK_EQUAL(fixture.actualFileName_, "");
        BOOST_CHECK(fixture.actualMask_ & event);
    }

    sleep(5);
}

BOOST_AUTO_TEST_SUITE_END()
