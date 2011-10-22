#include "MiningBundleConfiguration.h"

namespace sf1r
{
MiningBundleConfiguration::MiningBundleConfiguration(const std::string& collectionName)
    : ::izenelib::osgi::BundleConfiguration("MiningBundle-"+collectionName, "MiningBundleActivator" )
    , collectionName_(collectionName)
{
}
}
