#include "RecommendBundleConfiguration.h"

namespace sf1r
{

RecommendBundleConfiguration::RecommendBundleConfiguration(const std::string& collectionName)
    : ::izenelib::osgi::BundleConfiguration("RecommendBundle-"+collectionName, "RecommendBundleActivator" )
    , collectionName_(collectionName)
{
}

} // namespace sf1r
