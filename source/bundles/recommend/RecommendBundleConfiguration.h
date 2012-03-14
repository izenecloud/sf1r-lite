/**
 * @file RecommendBundleConfiguration.h
 * @author Jun Jiang
 * @date 2011-04-20
 */

#ifndef RECOMMEND_BUNDLE_CONFIGURATION_H
#define RECOMMEND_BUNDLE_CONFIGURATION_H

#include "RecommendSchema.h"
#include <configuration-manager/PropertyConfig.h>
#include <configuration-manager/CollectionPath.h>
#include <configuration-manager/CassandraStorageConfig.h>
#include <configuration-manager/DistributedNodeConfig.h>

#include <util/osgi/BundleConfiguration.h>

#include <string>
#include <iostream>

namespace sf1r
{

class RecommendBundleConfiguration : public ::izenelib::osgi::BundleConfiguration
{
public:
    RecommendBundleConfiguration(const std::string& collectionName);

    // <RecommendBundle><Schema>
    bool isSchemaEnable_;

    std::string collectionName_;

    CollectionPath collPath_;

    std::vector<std::string> collectionDataDirectories_;

    // <RecommendBundle><Parameter><CronPara>
    std::string cronStr_;

    // <RecommendBundle><Parameter><CacheSize>
    std::size_t purchaseCacheSize_;
    std::size_t visitCacheSize_;
    int64_t indexCacheSize_;

    // <RecommendBundle><Parameter><FreqItemSet>
    bool freqItemSetEnable_;
    std::size_t itemSetMinFreq_;

    // <RecommendBundle><Parameter><CassandraStorage>
    CassandraStorageConfig cassandraConfig_;

    // <Collection><RecommendBundle><Schema>
    RecommendSchema recommendSchema_;

    // <SF1Config><Deployment><DistributedTopology>
    DistributedNodeConfig recommendNodeConfig_;
    DistributedNodeConfig searchNodeConfig_;

    std::string userSCDPath() const
    {
        return collPath_.getScdPath() + "/recommend/user/";
    }

    std::string orderSCDPath() const
    {
        return collPath_.getScdPath() + "/recommend/order/";
    }

};

std::ostream& operator<<(std::ostream& out, const RecommendBundleConfiguration& config);

} // namespace sf1r

#endif // RECOMMEND_BUNDLE_CONFIGURATION_H
