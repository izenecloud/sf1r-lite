#ifndef MINING_BUNDLE_CONFIGURATION_H
#define MINING_BUNDLE_CONFIGURATION_H

#include <configuration-manager/MiningConfig.h>
#include <configuration-manager/MiningSchema.h>
#include <configuration-manager/CollectionPath.h>

#include <util/osgi/BundleConfiguration.h>

namespace sf1r
{
class MiningBundleConfiguration : public ::izenelib::osgi::BundleConfiguration
{
public:
    MiningBundleConfiguration(const std::string& collectionName);

    std::string collectionName_;

    CollectionPath collPath_;

    MiningConfig mining_config_;

    MiningSchema mining_schema_;
};
}

#endif

