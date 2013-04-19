#ifndef PRODUCT_BUNDLE_CONFIGURATION_H
#define PRODUCT_BUNDLE_CONFIGURATION_H

#include <configuration-manager/PropertyConfig.h>
#include <configuration-manager/CollectionPath.h>
#include <configuration-manager/CassandraStorageConfig.h>
#include <product-manager/pm_config.h>
#include <util/osgi/BundleConfiguration.h>
#include <util/ustring/UString.h>

namespace sf1r
{

class ProductBundleConfiguration : public ::izenelib::osgi::BundleConfiguration
{
public:
    ProductBundleConfiguration(const std::string& collectionName);

    void setSchema(const IndexBundleSchema& indexSchema);

public:
    std::string mode_;

    std::string collectionName_;

    std::string productId_;

    std::string callback_;

    std::string cron_;

    CollectionPath collPath_;

    IndexBundleSchema indexSchema_;

    std::vector<std::string> collectionDataDirectories_;

    PMConfig pm_config_;

    CassandraStorageConfig cassandraConfig_;
};
}

#endif
