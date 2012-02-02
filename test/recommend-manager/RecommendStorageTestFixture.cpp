#include "RecommendStorageTestFixture.h"
#include <log-manager/CassandraConnection.h>

#include <boost/filesystem.hpp>

namespace bfs = boost::filesystem;

namespace sf1r
{

const std::string RecommendStorageTestFixture::REMOTE_STORAGE_URL("cassandra://localhost");
const std::string RecommendStorageTestFixture::REMOTE_STORAGE_URL_NOT_CONNECT("cassandra://localhost:9161");

bool RecommendStorageTestFixture::initLocalStorage(
    const std::string& collection,
    const std::string& testDir
)
{
    bfs::remove_all(testDir);
    bfs::create_directories(testDir);

    cassandraConfig_.enable = false;
    cassandraConfig_.keyspace.clear();

    factory_.reset(new RecommendStorageFactory(cassandraConfig_, collection, testDir));

    return true;
}

bool RecommendStorageTestFixture::initRemoteStorage(
    const std::string& collection,
    const std::string& testDir,
    const std::string& keyspace,
    const std::string& testUrl
)
{
    CassandraConnection& connection = CassandraConnection::instance();
    bool connectResult = connection.init(testUrl);

    connection.dropKeyspace(keyspace);
    bfs::remove_all(testDir);
    bfs::create_directories(testDir);

    cassandraConfig_.enable = true;
    cassandraConfig_.keyspace = keyspace;

    factory_.reset(new RecommendStorageFactory(cassandraConfig_, collection, ""));

    return connectResult;
}

} // namespace sf1r
