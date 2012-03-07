#include "MiningBundleConfiguration.h"

namespace sf1r
{
std::string MiningBundleConfiguration::system_resource_path_;
std::string MiningBundleConfiguration::system_working_path_;

MiningBundleConfiguration::MiningBundleConfiguration(const std::string& collectionName)
    : ::izenelib::osgi::BundleConfiguration("MiningBundle-"+collectionName, "MiningBundleActivator" )
    , collectionName_(collectionName)
    , isSchemaEnable_(false)
{
}
}
