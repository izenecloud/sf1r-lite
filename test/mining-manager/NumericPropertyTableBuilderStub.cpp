#include "NumericPropertyTableBuilderStub.h"
#include <document-manager/Document.h>
#include <configuration-manager/GroupConfig.h>
#include <search-manager/NumericPropertyTable.h>

#include <glog/logging.h>

#include <cstring> // memcpy

namespace
{
using namespace sf1r;

template<typename T>
void* createPropertyData(
   typename NumericPropertyTableBuilderStub::PropertyMap<T>::map_type& propMap,
   const std::string& propName,
   size_t& size
)
{
    typename NumericPropertyTableBuilderStub::PropertyMap<T>::table_type& propTable = propMap[propName];
    std::size_t num = propTable.size();
    void* data = new T[num];
    memcpy(data, &propTable[0], num*sizeof(T));
    size = num;
    return data;
}

template<typename T>
void insertPropertyMap(
    const std::string& propName,
    const PropertyValue* propValue,
    typename NumericPropertyTableBuilderStub::PropertyMap<T>::map_type& propMap
)
{
    T value = 0;
    if (propValue)
        value = propValue->get<T>();

    propMap[propName].push_back(value);
}

}

namespace sf1r
{

NumericPropertyTableBuilderStub::NumericPropertyTableBuilderStub(const std::vector<GroupConfig>& groupConfigs)
    : groupConfigs_(groupConfigs)
    , lastDocId_(0)
{
    // insert property value for doc id 0
    for (std::vector<GroupConfig>::const_iterator it = groupConfigs_.begin();
        it != groupConfigs_.end(); ++it)
    {
        const std::string& propName = it->propName;
        PropertyDataType propType = it->propType;

        insertProperty_(propName, propType, NULL);
    }
}

PropertyDataType NumericPropertyTableBuilderStub::getPropertyType_(const std::string& prop) const
{
    for (std::vector<GroupConfig>::const_iterator it = groupConfigs_.begin();
        it != groupConfigs_.end(); ++it)
    {
        if (it->propName == prop)
            return it->propType;
    }

    return UNKNOWN_DATA_PROPERTY_TYPE;
}

NumericPropertyTable* NumericPropertyTableBuilderStub::createPropertyTable(const std::string& propertyName)
{
    PropertyDataType propType = getPropertyType_(propertyName);
    void* data = NULL;
    size_t size = 0;
    switch(propType)
    {
    case INT_PROPERTY_TYPE:
        data = createPropertyData<int64_t>(intPropMap_, propertyName, size);
        break;

    case UNSIGNED_INT_PROPERTY_TYPE:
        data = createPropertyData<uint64_t>(uintPropMap_, propertyName, size);
        break;

    case FLOAT_PROPERTY_TYPE:
        data = createPropertyData<float>(floatPropMap_, propertyName, size);
        break;

    case DOUBLE_PROPERTY_TYPE:
        data = createPropertyData<double>(doublePropMap_, propertyName, size);
        break;

    default:
        LOG(ERROR) << "unsupported type " << propType
                   << " for group property " << propertyName;
        break;
    }

    if (data)
    {
        boost::shared_ptr<PropertyData> propData(new PropertyData(propType, data, size));
        return new NumericPropertyTable(propertyName, propData);
    }

    return NULL;
}

bool NumericPropertyTableBuilderStub::insertDocument(const Document& doc)
{
    if (doc.getId() != lastDocId_+1)
    {
        LOG(ERROR) << "failed to insert doc id " << doc.getId()
                   << ", expect id " << (lastDocId_+1);
        return false;
    }
    ++lastDocId_;

    for (std::vector<GroupConfig>::const_iterator it = groupConfigs_.begin();
        it != groupConfigs_.end(); ++it)
    {
        const std::string& propName = it->propName;
        PropertyDataType propType = it->propType;
        const PropertyValue pv = doc.property(propName);

        if (!insertProperty_(propName, propType, &pv))
            return false;
    }

    return true;
}

bool NumericPropertyTableBuilderStub::insertProperty_(
    const std::string& prop,
    PropertyDataType type,
    const PropertyValue* propValue
)
{
    try
    {
        insertPropMap_(prop, type, propValue);
    }catch(const boost::bad_get& e)
    {
        LOG(ERROR) << "failed in converting type: " << type
                   << ", property: " << prop
                   << ", exception: " << e.what();
        return false;
    }

    return true;
}

void NumericPropertyTableBuilderStub::insertPropMap_(
    const std::string& prop,
    PropertyDataType type,
    const PropertyValue* propValue
)
{
    switch(type)
    {
    case INT_PROPERTY_TYPE:
        insertPropertyMap<int64_t>(prop, propValue, intPropMap_);
        break;

    case UNSIGNED_INT_PROPERTY_TYPE:
        insertPropertyMap<uint64_t>(prop, propValue, uintPropMap_);
        break;

    case FLOAT_PROPERTY_TYPE:
        insertPropertyMap<float>(prop, propValue, floatPropMap_);
        break;

    case DOUBLE_PROPERTY_TYPE:
        insertPropertyMap<double>(prop, propValue, doublePropMap_);
        break;

    default:
        break;
    }
}

}
