/**
 * @file RecommendStorageFactory.h
 * @brief create storage instances, such as UserManager, PurchaseManager, etc.
 * @author Jun Jiang
 * @date 2012-01-18
 */

#ifndef RECOMMEND_STORAGE_FACTORY_H
#define RECOMMEND_STORAGE_FACTORY_H

#include <string>

namespace libcassandra
{
class Cassandra;
}

namespace sf1r
{
struct CassandraStorageConfig;
class UserManager;
class PurchaseManager;

class RecommendStorageFactory
{
public:
    RecommendStorageFactory(
        const CassandraStorageConfig& cassandraConfig,
        const std::string& collection,
        const std::string& dataDir
    );

    UserManager* createUserManager() const;
    PurchaseManager* createPurchaseManager() const;

    const std::string& getUserColumnFamily() const;
    const std::string& getPurchaseColumnFamily() const;

private:
    void initRemoteStorage_(const std::string& collection);
    void initLocalStorage_(const std::string& dataDir);

private:
    const CassandraStorageConfig& cassandraConfig_;
    libcassandra::Cassandra* cassandraClient_;

    std::string userPath_;
    std::string purchasePath_;

    std::string userColumnFamily_;
    std::string purchaseColumnFamily_;
};

} // namespace sf1r

#endif // RECOMMEND_STORAGE_FACTORY_H
