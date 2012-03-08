#include "IndexBundleConfiguration.h"

#include <boost/algorithm/string.hpp>

namespace sf1r
{
IndexBundleConfiguration::IndexBundleConfiguration(const std::string& collectionName)
    : ::izenelib::osgi::BundleConfiguration("IndexBundle-"+collectionName, "IndexBundleActivator" )
    , collectionName_(collectionName)
    , isSchemaEnable_(false)
    , logCreatedDoc_(false)
    , bIndexUnigramProperty_(true)
    , bUnigramSearchMode_(false)
    , indexMultilangGranularity_(la::FIELD_LEVEL)
    , isAutoRebuild_(false)
    , encoding_(izenelib::util::UString::UNKNOWN)
    , wildcardType_("unigram")
{}

void IndexBundleConfiguration::setSchema(const DocumentSchema& documentSchema)
{
    documentSchema_ = documentSchema;
    
    for(DocumentSchema::const_iterator iter = documentSchema_.begin();
        iter != documentSchema_.end(); ++iter)
    {
        PropertyConfig property(*iter);
        indexSchema_.insert(property);
        string pName = iter->propertyName_;
        boost::to_lower(pName);
        if(pName == "date" )
        {
            // always index and filterable
            PropertyConfig updatedDatePropertyConfig(*iter);
            updatedDatePropertyConfig.setIsIndex(true);
            updatedDatePropertyConfig.setIsFilter(true);
            eraseProperty(iter->propertyName_);
            indexSchema_.insert(updatedDatePropertyConfig);
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
    propertyid_t id = 1;
    for (IndexBundleSchema::iterator it = indexSchema_.begin(), itEnd = indexSchema_.end();
        it != itEnd; ++it)
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

    IndexBundleSchema::const_iterator it(indexSchema_.find(byName));
    if (it != indexSchema_.end())
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

    IndexBundleSchema::const_iterator iter = indexSchema_.find(config);

    if (iter != indexSchema_.end() )
    {
        analysisInfo = iter->analysisInfo_;
        analysis = analysisInfo.analyzerId_;
        language = "";

        return true;
    }

    return false;
}

}
