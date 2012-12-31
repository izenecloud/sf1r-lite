#include "NumericPropertyTableBuilderStub.h"
#include <document-manager/Document.h>
#include <common/NumericPropertyTable.h>

#include <glog/logging.h>

#include <cstring> // memcpy

namespace
{
using namespace sf1r;

template<typename T>
void createPropertyData(
        PropertyDataType type,
        typename NumericPropertyTableBuilderStub::PropertyMap<T>::map_type& propMap,
        const std::string& propName,
        boost::shared_ptr<NumericPropertyTableBase>& numericPropertyTable)
{
    if (numericPropertyTable)
        return;

    numericPropertyTable.reset(new NumericPropertyTable<T>(type));
    typename NumericPropertyTableBuilderStub::PropertyMap<T>::table_type&
        propTable = propMap[propName];

    std::size_t num = propTable.size();
    numericPropertyTable->resize(num);
    void* data = numericPropertyTable->getValueList();
    size_t size = num*sizeof(T);
    memcpy(data, &propTable[0], size);
}

template<typename T>
void insertPropertyMap(
        const std::string& propName,
        const PropertyValue* propValue,
        typename NumericPropertyTableBuilderStub::PropertyMap<T>::map_type& propMap)
{
    T value = 0;
    if (propValue)
        value = propValue->get<T>();

    propMap[propName].push_back(value);
}

}

namespace sf1r
{

NumericPropertyTableBuilderStub::NumericPropertyTableBuilderStub(const GroupConfigMap& groupConfigMap)
    : groupConfigMap_(groupConfigMap)
    , lastDocId_(0)
{
    // insert property value for doc id 0
    for (GroupConfigMap::const_iterator it = groupConfigMap_.begin();
        it != groupConfigMap_.end(); ++it)
    {
        const std::string& propName = it->first;
        PropertyDataType propType = it->second.propType;

        insertProperty_(propName, propType, NULL);
    }
}

PropertyDataType NumericPropertyTableBuilderStub::getPropertyType_(const std::string& prop) const
{
    GroupConfigMap::const_iterator it = groupConfigMap_.find(prop);

    if (it != groupConfigMap_.end())
        return it->second.propType;

    return UNKNOWN_DATA_PROPERTY_TYPE;
}

boost::shared_ptr<NumericPropertyTableBase>& NumericPropertyTableBuilderStub::createPropertyTable(const std::string& propertyName)
{
    PropertyDataType propType = getPropertyType_(propertyName);
    boost::shared_ptr<NumericPropertyTableBase>& numericPropertyTable = numericPropertyTableMap_[propertyName];

    switch (propType)
    {
    case INT32_PROPERTY_TYPE:
        createPropertyData<int32_t>(INT32_PROPERTY_TYPE, int32PropMap_, propertyName, numericPropertyTable);
        break;

    case INT64_PROPERTY_TYPE:
	case DATETIME_PROPERTY_TYPE:
        createPropertyData<int64_t>(INT64_PROPERTY_TYPE, int64PropMap_, propertyName, numericPropertyTable);
        break;

    case FLOAT_PROPERTY_TYPE:
        createPropertyData<float>(FLOAT_PROPERTY_TYPE, floatPropMap_, propertyName, numericPropertyTable);
        break;

    case DOUBLE_PROPERTY_TYPE:
        createPropertyData<double>(DOUBLE_PROPERTY_TYPE, doublePropMap_, propertyName, numericPropertyTable);
        break;

    default:
        LOG(ERROR) << "unsupported type " << propType
                   << " for group property " << propertyName;
        break;
    }

    return numericPropertyTable;
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

    for (GroupConfigMap::const_iterator it = groupConfigMap_.begin();
        it != groupConfigMap_.end(); ++it)
    {
        const std::string& propName = it->first;
        PropertyDataType propType = it->second.propType;
        const PropertyValue pv = doc.property(propName);

        if (!insertProperty_(propName, propType, &pv))
            return false;
    }

    return true;
}

bool NumericPropertyTableBuilderStub::insertProperty_(
        const std::string& prop,
        PropertyDataType type,
        const PropertyValue* propValue)
{
    try
    {
        insertPropMap_(prop, type, propValue);
    }
    catch (const boost::bad_get& e)
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
        const PropertyValue* propValue)
{
    switch (type)
    {
    case INT32_PROPERTY_TYPE:
        insertPropertyMap<int32_t>(prop, propValue, int32PropMap_);
        break;

    case INT64_PROPERTY_TYPE:
        insertPropertyMap<int64_t>(prop, propValue, int64PropMap_);
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

void NumericPropertyTableBuilderStub::clearTableMap()
{
    numericPropertyTableMap_.clear();
}

}
