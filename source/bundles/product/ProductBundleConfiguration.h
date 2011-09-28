#ifndef PRODUCT_BUNDLE_CONFIGURATION_H
#define PRODUCT_BUNDLE_CONFIGURATION_H

#include <configuration-manager/PropertyConfig.h>
#include <configuration-manager/CollectionPath.h>

#include <util/osgi/BundleConfiguration.h>
#include <util/ustring/UString.h>

namespace sf1r
{
typedef std::set<PropertyConfig, PropertyComp> BundleSchema;

class ProductBundleConfiguration : public ::izenelib::osgi::BundleConfiguration
{
public:
    ProductBundleConfiguration(const std::string& collectionName);

    void setSchema(const std::set<PropertyConfigBase, PropertyBaseComp>& schema);

private:

    bool eraseProperty(const std::string& name)
    {
        PropertyConfig config;
        config.propertyName_ = name;
        return schema_.erase(config);
    }

public:
    bool enabled_;

    std::string collectionName_;

    CollectionPath collPath_;
	
    /// Schema
    std::set<PropertyConfig, PropertyComp> schema_;

    /// @brief The encoding type of the Collection
    izenelib::util::UString::EncodingType encoding_;

    std::vector<std::string> collectionDataDirectories_;
};
}

#endif

