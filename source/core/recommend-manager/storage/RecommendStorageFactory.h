/**
 * @file RecommendStorageFactory.h
 * @brief create storage instances, such as UserManager, PurchaseManager, etc.
 * @author Jun Jiang
 * @date 2012-01-18
 */

#ifndef RECOMMEND_STORAGE_FACTORY_H
#define RECOMMEND_STORAGE_FACTORY_H

#include <string>
#include <boost/filesystem.hpp>

namespace sf1r
{
class UserManager;
struct CassandraStorageConfig;

class RecommendStorageFactory
{
public:
    RecommendStorageFactory(
        const std::string& collection,
        const std::string& dataDir,
        const CassandraStorageConfig& cassandraConfig
    );

    UserManager* createUserManager() const;

private:
    const std::string collection_;
    const boost::filesystem::path userDir_;
    const CassandraStorageConfig& cassandraConfig_;
};

} // namespace sf1r

#endif // RECOMMEND_STORAGE_FACTORY_H
