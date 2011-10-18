#include "ProductBundleConfiguration.h"

#include <boost/algorithm/string.hpp>

namespace sf1r
{
ProductBundleConfiguration::ProductBundleConfiguration(const std::string& collectionName)
    : ::izenelib::osgi::BundleConfiguration("ProductBundle-"+collectionName, "ProductBundleActivator" )
    , enabled_(false)
    , collectionName_(collectionName)
{}

void ProductBundleConfiguration::setSchema(const std::set<PropertyConfigBase, PropertyBaseComp>& schema)
{
    std::set<PropertyConfigBase, PropertyBaseComp>::const_iterator iter = schema.begin();
    for(; iter != schema.end(); ++iter)
    {
        PropertyConfig property(*iter);
        schema_.insert(property);
    }
}


}

