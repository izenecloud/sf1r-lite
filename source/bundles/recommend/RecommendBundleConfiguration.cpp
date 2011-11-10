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

} // namespace sf1r
