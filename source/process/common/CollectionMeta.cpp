#include "CollectionMeta.h"

#include <sstream>

namespace sf1r
{

CollectionMeta::CollectionMeta()
    : colId_(0)
{}

bool CollectionMeta::getProperty(PropertyConfigBase& config) const
{
    DocumentSchema::const_iterator it(documentSchema_.find(config));
    if (it != documentSchema_.end())
    {
        config = *it;

        return true;
    }

    return false;
}

bool CollectionMeta::getProperty(const std::string& name, PropertyConfigBase& config) const
{
    PropertyConfigBase byName;
    byName.propertyName_ = name;

    DocumentSchema::const_iterator it(documentSchema_.find(byName));
    if (it != documentSchema_.end())
    {
        config = *it;
        return true;
    }
    return false;
}

bool CollectionMeta::getPropertyType(const std::string& name, PropertyDataType& type) const
{
    PropertyConfigBase config;
    if ( !getProperty(name, config)) return false;
    type = config.propertyType_;
    return true;
}

std::string CollectionMeta::toString() const
{
    std::stringstream sStream;

    sStream << "[CollectionMetaData] @id=" << colId_
    << " @name=" << name_
    << " @documentSchema count=" << documentSchema_.size()
    << " @documentSchema " << std::endl;

    for (DocumentSchema::const_iterator it = documentSchema_.begin(), itEnd = documentSchema_.end();
        it != itEnd; ++it)
    {
        sStream << it->propertyName_<< std::endl;
    }

    return sStream.str();
}

void CollectionMeta::numberProperty()
{
    // we know that changing id will not affect the sequence, it is safe to use
    // const_cast and set the id.

    propertyid_t id = 1;
    for (DocumentSchema::iterator it = documentSchema_.begin(), itEnd = documentSchema_.end();
        it != itEnd; ++it)
    {
        PropertyConfigBase& config = const_cast<PropertyConfigBase&>(*it);
        config.propertyId_ = id++;
    }
}

}
