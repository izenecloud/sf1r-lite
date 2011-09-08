#include "NumericPropertyTableBuilderStub.h"
#include <document-manager/Document.h>
#include <configuration-manager/GroupConfig.h>
#include <search-manager/NumericPropertyTable.h>

#include <glog/logging.h>

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

    switch(propType)
    {
    case INT_PROPERTY_TYPE:
        data = &intPropMap_[propertyName][0];
        break;

    case UNSIGNED_INT_PROPERTY_TYPE:
        data = &uintPropMap_[propertyName][0];
        break;

    case FLOAT_PROPERTY_TYPE:
        data = &floatPropMap_[propertyName][0];
        break;

    case DOUBLE_PROPERTY_TYPE:
        data = &doublePropMap_[propertyName][0];
        break;

    default:
        LOG(ERROR) << "unsupported type " << propType
                   << " for group property " << propertyName;
        break;
    }

    if (data)
        return new NumericPropertyTable(propertyName, propType, data);

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
        {
            int64_t value = 0;
            if (propValue)
                value = propValue->get<int64_t>();

            intPropMap_[prop].push_back(value);
            break;
        }

        case UNSIGNED_INT_PROPERTY_TYPE:
        {
            uint64_t value = 0;
            if (propValue)
                value = propValue->get<uint64_t>();

            uintPropMap_[prop].push_back(value);
            break;
        }

        case FLOAT_PROPERTY_TYPE:
        {
            float value = 0;
            if (propValue)
                value = propValue->get<float>();

            floatPropMap_[prop].push_back(value);
            break;
        }

        case DOUBLE_PROPERTY_TYPE:
        {
            double value = 0;
            if (propValue)
                value = propValue->get<double>();

            doublePropMap_[prop].push_back(value);
            break;
        }

        default:
            break;
    }
}

}
