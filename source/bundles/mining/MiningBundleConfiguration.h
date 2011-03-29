#ifndef MINING_BUNDLE_CONFIGURATION_H
#define MINING_BUNDLE_CONFIGURATION_H

#include <configuration-manager/MiningConfig.h>
#include <configuration-manager/MiningSchema.h>

#include <util/osgi/BundleConfiguration.h>

namespace sf1r
{
class MiningBundleConfiguration : public ::izenelib::osgi::BundleConfiguration
{
public:
    MiningBundleConfiguration(const std::string& collectionName);

    MiningConfig mining_config_;

    MiningSchema mining_schema_;
};
}

#endif

