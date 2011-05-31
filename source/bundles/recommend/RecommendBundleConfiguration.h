/**
 * @file RecommendBundleConfiguration.h
 * @author Jun Jiang
 * @date 2011-04-20
 */

#ifndef RECOMMEND_BUNDLE_CONFIGURATION_H
#define RECOMMEND_BUNDLE_CONFIGURATION_H

#include "RecommendSchema.h"
#include <configuration-manager/PropertyConfig.h>
#include <configuration-manager/CollectionPath.h>

#include <util/osgi/BundleConfiguration.h>

#include <string>

namespace sf1r
{

class RecommendBundleConfiguration : public ::izenelib::osgi::BundleConfiguration
{
public:
    RecommendBundleConfiguration(const std::string& collectionName);

    std::string collectionName_;

    CollectionPath collPath_;

    std::vector<std::string> collectionDataDirectories_;

    // <CronPara> in "sf1config.xml"
    std::string cronStr_;

    // recommend schema in "collection.xml"
    RecommendSchema recommendSchema_;

    std::string userSCDPath() const
    {
        return collPath_.getScdPath() + "/recommend/user/";
    }

    std::string itemSCDPath() const
    {
        return collPath_.getScdPath() + "/recommend/item/";
    }

    std::string orderSCDPath() const
    {
        return collPath_.getScdPath() + "/recommend/order/";
    }

};

} // namespace sf1r

#endif // RECOMMEND_BUNDLE_CONFIGURATION_H
