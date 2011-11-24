#include "IndexBundleConfiguration.h"

#include <boost/algorithm/string.hpp>

namespace sf1r
{
IndexBundleConfiguration::IndexBundleConfiguration(const std::string& collectionName)
    : ::izenelib::osgi::BundleConfiguration("IndexBundle-"+collectionName, "IndexBundleActivator" )
    , collectionName_(collectionName)
    , bIndexUnigramProperty_(true)
    , bUnigramSearchMode_(false)
    , indexMultilangGranularity_(la::FIELD_LEVEL)
    , encoding_(izenelib::util::UString::UNKNOWN)
    , wildcardType_("unigram")
{}

void IndexBundleConfiguration::setSchema(
    const std::set<PropertyConfigBase, PropertyBaseComp>& schema
)
{
    std::set<PropertyConfigBase, PropertyBaseComp>::const_iterator iter = schema.begin();
    for(; iter != schema.end(); ++iter)
    {
        PropertyConfig property(*iter);
        schema_.insert(property);
        string pName = iter->propertyName_;
        boost::to_lower(pName);
        if(pName == "date" )
        {
            // always index and filterable
            PropertyConfig updatedDatePropertyConfig(*iter);
            updatedDatePropertyConfig.setIsIndex(true);
            updatedDatePropertyConfig.setIsFilter(true);
            eraseProperty(iter->propertyName_);
            schema_.insert(updatedDatePropertyConfig);
        }
    }
}

void IndexBundleConfiguration::setIndexMultiLangGranularity(
    const std::string& granularity
)
{
    if(! granularity.compare("sentence")) indexMultilangGranularity_ = la::SENTENCE_LEVEL;
    else if(! granularity.compare("block")) indexMultilangGranularity_ = la::BLOCK_LEVEL;
    else indexMultilangGranularity_ = la::FIELD_LEVEL;
}

void IndexBundleConfiguration::numberProperty()
{
    typedef std::set<PropertyConfig, PropertyComp>::iterator iterator;

    propertyid_t id = 1;
    for (iterator it = schema_.begin(), itEnd = schema_.end(); it != itEnd; ++it)
    {
        PropertyConfig& config = const_cast<PropertyConfig&>(*it);
        config.setPropertyId(id++);
    }
}

bool IndexBundleConfiguration::getPropertyConfig(
    const std::string& name,
    PropertyConfig& config
) const
{
    PropertyConfig byName;
    byName.setName(name);

    IndexBundleSchema::const_iterator it(schema_.find(byName));
    if (it != schema_.end())
    {
        config = *it;
        return true;
    }
    return false;
}

bool IndexBundleConfiguration::getAnalysisInfo(
    const std::string& propertyName,
    AnalysisInfo& analysisInfo,
    std::string& analysis,
    std::string& language
) const
{
    PropertyConfig config;
    config.setName(propertyName);

    IndexBundleSchema::iterator iter = schema_.find(config);

    if (iter != schema_.end() )
    {
        analysisInfo = iter->analysisInfo_;
        analysis = analysisInfo.analyzerId_;
        language = "";

        return true;
    }

    return false;
}

}
