#include "FileMonitorTestFixture.h"
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include <cstdlib> // system
#include <glog/logging.h>

using namespace sf1r;
namespace bfs = boost::filesystem;

FileMonitorTestFixture::FileMonitorTestFixture()
    : dirName_("file_monitor_test")
    , actualMask_(0)
{
    bfs::path dir(dirName_);
    bfs::remove_all(dir);
    bfs::create_directory(dir);
}

bool FileMonitorTestFixture::handle(const std::string& fileName, uint32_t mask)
{
    actualFileName_ = fileName;
    actualMask_ = mask;
    return true;
}

void FileMonitorTestFixture::start()
{
    monitor_.setFileEventHandler(this);
    monitor_.monitor();
}

void FileMonitorTestFixture::resetStatus()
{
    actualFileName_ = "";
    actualMask_ = 0;
}

void FileMonitorTestFixture::checkCommand(
    const std::string& command,
    const std::string& goldFileName,
    uint32_t goldMask)
{
    LOG(INFO) << "execute command: " << command;
    BOOST_REQUIRE_EQUAL(std::system(command.c_str()), 0);

    // wait for event is triggered and processed
    sleep(1);

    BOOST_CHECK_EQUAL(actualFileName_, goldFileName);
    BOOST_CHECK(actualMask_ & goldMask);
}
