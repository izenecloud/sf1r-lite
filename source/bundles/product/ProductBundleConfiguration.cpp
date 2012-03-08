#include "ProductBundleConfiguration.h"

#include <boost/algorithm/string.hpp>

namespace sf1r
{
ProductBundleConfiguration::ProductBundleConfiguration(const std::string& collectionName)
    : ::izenelib::osgi::BundleConfiguration("ProductBundle-"+collectionName, "ProductBundleActivator" )
    , mode_(0)
    , collectionName_(collectionName)
    , pm_config_()
{}

void ProductBundleConfiguration::setSchema(const IndexBundleSchema& indexSchema)
{
    indexSchema_ = indexSchema;
}


}
