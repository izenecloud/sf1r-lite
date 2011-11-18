#ifndef PRODUCT_BUNDLE_CONFIGURATION_H
#define PRODUCT_BUNDLE_CONFIGURATION_H

#include <configuration-manager/PropertyConfig.h>
#include <configuration-manager/CollectionPath.h>
#include <product-manager/pm_config.h>
#include <util/osgi/BundleConfiguration.h>
#include <util/ustring/UString.h>

namespace sf1r
{

class ProductBundleConfiguration : public ::izenelib::osgi::BundleConfiguration
{
public:
    ProductBundleConfiguration(const std::string& collectionName);

    void setSchema(const std::set<PropertyConfig, PropertyComp>& schema);

public:
    int mode_;

    std::string collectionName_;

    CollectionPath collPath_;

    std::set<PropertyConfig, PropertyComp> schema_;

    std::vector<std::string> collectionDataDirectories_;

    PMConfig pm_config_;

};
}

#endif
