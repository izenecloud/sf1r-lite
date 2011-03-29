#include "MiningBundleConfiguration.h"

namespace sf1r
{
MiningBundleConfiguration::MiningBundleConfiguration(const std::string& collectionName)
    : ::izenelib::osgi::BundleConfiguration(collectionName+"-mining", "MiningBundleActivator" )
{
}
}

