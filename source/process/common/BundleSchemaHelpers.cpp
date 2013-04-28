/**
 * @file core/common/BundleSchemaHelpers.cpp
 * @author Ian Yang
 * @author Kevin Lin
 * @date Created <2010-07-12 11:19:38>
 * @date Updated <2013-04-27>
 */
#include "BundleSchemaHelpers.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string.hpp>
#include <glog/logging.h>

namespace sf1r {

bool getPropertyConfig(
    const IndexBundleSchema& schema,
    PropertyConfig& config
)
{
    IndexBundleSchema::const_iterator it(schema.find(config));
    if (it != schema.end())
    {
        config = *it;

        return true;
    }

    return false;
}

bool getPropertyConfig(
    const IndexBundleSchema& schema,
    const std::string& name,
    PropertyConfig& config)
{
    PropertyConfig byName;
    byName.setName(name);

    IndexBundleSchema::const_iterator it(schema.find(byName));
    if (it != schema.end())
    {
        config = *it;

        return true;
    }

    return false;
}

void getDefaultSearchPropertyNames(
    const IndexBundleSchema& schema,
    std::vector<std::string>& names
)
{
    std::vector<std::string> result;
    typedef IndexBundleSchema::const_iterator iterator;
    for (iterator it = schema.begin();
         it != schema.end(); ++it)
    {
        if (it->bIndex_&&it->subProperties_.empty()&&(!it->isRTypeString())&&(it->getType() == STRING_PROPERTY_TYPE))
        {
            result.push_back(it->propertyName_);
        }
    }

    result.swap(names);
}

void getDefaultSelectPropertyNames(
    const IndexBundleSchema& schema,
    std::vector<std::string>& names
)
{
    std::vector<std::string> result;
    typedef IndexBundleSchema::const_iterator iterator;
    for (iterator it = schema.begin();
         it != schema.end(); ++it)
    {
        // ignore alias
        if (!it->isAliasProperty())
        {
            result.push_back(it->propertyName_);
        }
    }

    result.swap(names);
}
bool isPropertySortable(
    const IndexBundleSchema& schema,
    const std::string& property
)
{
    std::string lowerPropertyName = boost::to_lower_copy(property);
    if (lowerPropertyName != "_rank" && lowerPropertyName != "date")
    {
        PropertyConfig propertyConfig;
        propertyConfig.setName(property);

        IndexBundleSchema::iterator it = schema.find(propertyConfig);
        if (it == schema.end() || 
            (!it->isRTypeNumeric() && 
            !it->isRTypeString()))
            return false;
    }

    return true;
}

bool isPropertyFilterable(
    const IndexBundleSchema& schema,
    const std::string& property
)
{
    std::string lowerPropertyName = boost::to_lower_copy(property);
    if (lowerPropertyName != "_rank" && lowerPropertyName != "date")
    {
        PropertyConfig propertyConfig;
        propertyConfig.setName(property);

        IndexBundleSchema::iterator it = schema.find(propertyConfig);
        if (it == schema.end())
        {
            return false;
        }
        propertyConfig = *it;

        if (!propertyConfig.bIndex_ ||
            !propertyConfig.bFilter_)
        {
            return false;
        }
    }

    return true;
}

sf1r::PropertyDataType getPropertyDataType(
    const IndexBundleSchema& schema,
    const std::string& property
)
{
    std::string lowerPropertyName = boost::to_lower_copy(property);
    if (lowerPropertyName == "_rank")
    {
        return sf1r::FLOAT_PROPERTY_TYPE;
    }
    else if (lowerPropertyName == "date")
    {
        return sf1r::DATETIME_PROPERTY_TYPE;
    }
    else
    {
        PropertyConfig propertyConfig;
        propertyConfig.setName(property);
        IndexBundleSchema::iterator it = schema.find(propertyConfig);
        if (it != schema.end())
        {
            return it->propertyType_;
        }
    }

    return sf1r::UNKNOWN_DATA_PROPERTY_TYPE;
}

bool isDocumentProperty(
    const DocumentSchema& schema,
    const std::string& property
)
{
    PropertyConfigBase config;
    config.propertyName_ = property;

    return schema.find(config) != schema.end();
}

void getDocumentPropertyNames(
    const DocumentSchema& schema,
    std::vector<std::string>& names
)
{
    for (DocumentSchema::const_iterator it = schema.begin();
        it != schema.end(); ++it)
    {
        names.push_back(it->propertyName_);
    }
}

} // NAMESPACE sf1r
