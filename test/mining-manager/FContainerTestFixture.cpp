#include "FContainerTestFixture.h"
#include <boost/filesystem.hpp>

namespace bfs = boost::filesystem;

namespace
{
const char* TEST_DIR = "fcontainer_test";
}

namespace sf1r
{

FContainerTestFixture::FContainerTestFixture()
    : testDir_(TEST_DIR)
{
    bfs::path testPath(testDir_);
    bfs::remove_all(testPath);
    bfs::create_directory(testPath);
}

} // namespace sf1r
