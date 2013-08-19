#include "FileMonitorTestFixture.h"
#include <boost/filesystem.hpp>

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
