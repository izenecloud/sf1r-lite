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

namespace sf1r
{

class RecommendBundleConfiguration : public ::izenelib::osgi::BundleConfiguration
{
public:
    RecommendBundleConfiguration(const std::string& collectionName);

    std::string collectionName_;

    CollectionPath collPath_;
	
    std::vector<std::string> collectionDataDirectories_;

    // TODO recommend parameter in "sf1config.xml"
    /*RecommendParam recommendParam_;*/

    // recommend schema in "collection.xml"
    RecommendSchema recommendSchema_;
};

} // namespace sf1r

#endif // RECOMMEND_BUNDLE_CONFIGURATION_H
