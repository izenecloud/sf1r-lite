#include "RecommendBundleConfiguration.h"

namespace sf1r
{

RecommendBundleConfiguration::RecommendBundleConfiguration(const std::string& collectionName)
    : ::izenelib::osgi::BundleConfiguration("RecommendBundle-"+collectionName, "RecommendBundleActivator" )
    , isSchemaEnable_(false)
    , collectionName_(collectionName)
    , purchaseCacheSize_(0x40000000) // 1G
    , visitCacheSize_(0x20000000) // 512M
    , indexCacheSize_(0xA00000) // 10M
    , freqItemSetEnable_(false)
    , itemSetMinFreq_(10)
{
}

std::ostream& operator<<(std::ostream& out, const RecommendBundleConfiguration& config)
{
    out << "[" << config.collectionName_
        << "] CronPara: " << config.cronStr_
        << "\n[CacheSize] purchase: " << config.purchaseCacheSize_
        << ", visit: " << config.visitCacheSize_
        << ", index: " << config.indexCacheSize_
        << "\n[FreqItemSet] enable: " << config.freqItemSetEnable_
        << ", min freq: " << config.itemSetMinFreq_
        << "\n[CassandraStorage] enable: " << config.cassandraConfig_.enable
        << ", keyspace: " << config.cassandraConfig_.keyspace;

    return out;
}

} // namespace sf1r
