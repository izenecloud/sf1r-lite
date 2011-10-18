#include "ProductBundleConfiguration.h"

#include <boost/algorithm/string.hpp>

namespace sf1r
{
ProductBundleConfiguration::ProductBundleConfiguration(const std::string& collectionName)
    : ::izenelib::osgi::BundleConfiguration("ProductBundle-"+collectionName, "ProductBundleActivator" )
    , enabled_(false)
    , collectionName_(collectionName)
{}


}

