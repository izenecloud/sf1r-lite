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
class VisitManager;

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
    VisitManager* createVisitManager() const;

    const std::string& getUserColumnFamily() const;
    const std::string& getPurchaseColumnFamily() const;
    const std::string& getVisitItemColumnFamily() const;
    const std::string& getVisitRecommendColumnFamily() const;
    const std::string& getVisitSessionColumnFamily() const;

private:
    void initRemoteStorage_(const std::string& collection);
    void initLocalStorage_(const std::string& dataDir);

private:
    const CassandraStorageConfig& cassandraConfig_;
    libcassandra::Cassandra* cassandraClient_;

    std::string userPath_;
    std::string purchasePath_;
    std::string visitItemPath_;
    std::string visitRecommendPath_;
    std::string visitSessionPath_;

    std::string userColumnFamily_;
    std::string purchaseColumnFamily_;
    std::string visitItemColumnFamily_;
    std::string visitRecommendColumnFamily_;
    std::string visitSessionColumnFamily_;
};

} // namespace sf1r

#endif // RECOMMEND_STORAGE_FACTORY_H
