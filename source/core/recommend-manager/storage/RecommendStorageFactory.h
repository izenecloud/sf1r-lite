/**
 * @file RecommendStorageFactory.h
 * @brief create storage instances, such as UserManager, PurchaseManager, etc.
 * @author Jun Jiang
 * @date 2012-01-18
 */

#ifndef RECOMMEND_STORAGE_FACTORY_H
#define RECOMMEND_STORAGE_FACTORY_H

#include <string>
#include <vector>

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
class CartManager;
class RateManager;

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
    CartManager* createCartManager() const;
    RateManager* createRateManager() const;

private:
    void initRemoteStorage_(const std::string& collection);
    void initLocalStorage_(const std::string& dataDir);

private:
    const CassandraStorageConfig& cassandraConfig_;
    libcassandra::Cassandra* cassandraClient_;

    /**
     * storage path.
     * for local storage, it means local file path;
     * for remote storage, it means column family name in cassandra.
     */
    enum StoragePathID
    {
        STORAGE_PATH_ID_USER = 0,
        STORAGE_PATH_ID_PURCHASE,
        STORAGE_PATH_ID_VISIT_ITEM,
        STORAGE_PATH_ID_VISIT_RECOMMEND,
        STORAGE_PATH_ID_VISIT_SESSION,
        STORAGE_PATH_ID_CART,
        STORAGE_PATH_ID_RATE,
        STORAGE_PATH_ID_NUM
    };

    /** key: StoragePathID */
    std::vector<std::string> storagePaths_;
};

} // namespace sf1r

#endif // RECOMMEND_STORAGE_FACTORY_H
