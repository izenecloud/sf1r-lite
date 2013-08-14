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
