///
/// @file RecommendStorageTestFixture.h
/// @brief base fixture class to test recommend storage, such as UserManager, PurchaseManager, etc
/// @author Jun Jiang
/// @date Created 2012-02-01
///

#ifndef RECOMMEND_STORAGE_TEST_FIXTURE
#define RECOMMEND_STORAGE_TEST_FIXTURE

#include <configuration-manager/CassandraStorageConfig.h>
#include <recommend-manager/storage/RecommendStorageFactory.h>

#include <string>
#include <boost/scoped_ptr.hpp>

namespace sf1r
{

class RecommendStorageTestFixture
{
public:
    virtual ~RecommendStorageTestFixture() {}

    bool initLocalStorage(
        const std::string& collection,
        const std::string& testDir
    );

    bool initRemoteStorage(
        const std::string& collection,
        const std::string& testDir,
        const std::string& keyspace,
        const std::string& testUrl
    );

    virtual void resetInstance() = 0;

    static const std::string REMOTE_STORAGE_URL;
    static const std::string REMOTE_STORAGE_URL_NOT_CONNECT;

protected:
    CassandraStorageConfig cassandraConfig_;

    boost::scoped_ptr<RecommendStorageFactory> factory_;
};

} // namespace sf1r

#endif //RECOMMEND_STORAGE_TEST_FIXTURE
