#include "CollectionMeta.h"

#include <sstream>

namespace sf1r
{

CollectionMeta::CollectionMeta()
: colId_(0)
, name_()
, encoding_(izenelib::util::UString::UNKNOWN)
, rankingModel_()
, wildcardType_("unigram")
, dateFormat_()
, collPath_()
, schema_()
, acl_()
{}

bool CollectionMeta::getPropertyConfig(const std::string& name,
                                       PropertyConfig& config) const
{
    PropertyConfig byName;
    byName.setName(name);

    property_config_const_iterator it(schema_.find(byName));
    if (it != schema_.end())
    {
        config = *it;

        return true;
    }

    return false;
}

bool CollectionMeta::getPropertyType(const std::string& name, PropertyDataType& type) const
{
  PropertyConfig config;
  if( !getPropertyConfig(name, config)) return false;
  type = config.getType();
  return true;
}

bool CollectionMeta::getPropertyConfig(PropertyConfig& config) const
{
    property_config_const_iterator it(schema_.find(config));
    if (it != schema_.end())
    {
        config = *it;

        return true;
    }

    return false;
}

bool CollectionMeta::getAnalysisInfo(
    const std::string& propertyName,
    AnalysisInfo&      analysisInfo,
    std::string&            analysis,
    std::string&            language
) const
{
    PropertyConfig config;
    config.setName(propertyName);

    if (getPropertyConfig(config))
    {
        analysisInfo = config.getAnalysisInfo();

        analysis = analysisInfo.analyzerId_;
        language = "";

        return true;
    }

    return false;
}

bool CollectionMeta:: getAttributesInDisplayElement(
    const std::string& propertyName,
    bool& bSnippet,
    bool& bHighlight,
    bool& bSummary
) const
{
    PropertyConfig config;
    config.setName(propertyName);

    if (getPropertyConfig(config))
    {
        bSnippet = config.getIsSnippet();
        bHighlight = false;
        //bHighlight = config.getIsHighlight();
        bSummary = config.getIsSummary();

        return true;
    }

    return false;
}


std::string CollectionMeta::toString() const
{
    std::stringstream sStream;

    sStream << "[CollectionMetaData] @id=" << colId_
            << " @name=" << name_
            << " @encoding=" << encoding_
            << " @wildcardType=" << wildcardType_
            << " @ranking=" << rankingModel_
            << " @documentSchema count=" << schema_.size()
            << " @documentSchema " << std::endl;

    typedef std::set<PropertyConfig, PropertyComp>::iterator iterator;
    for (iterator it = schema_.begin(),
               itEnd = schema_.end();
         it != itEnd; ++it)
    {
        PropertyConfig& config = const_cast<PropertyConfig&>(*it);
        sStream << config.toString() << std::endl;
    }

    return sStream.str();
}

void CollectionMeta::numberPropertyConfig()
{
    // we know that changing id will not affect the sequence, it is safe to use
    // const_cast and set the id.

    typedef std::set<PropertyConfig, PropertyComp>::iterator iterator;

    propertyid_t id = 1;
    for (iterator it = schema_.begin(),
               itEnd = schema_.end();
         it != itEnd; ++it)
    {
        PropertyConfig& config = const_cast<PropertyConfig&>(*it);
        config.setPropertyId(id++);
    }
}

}
