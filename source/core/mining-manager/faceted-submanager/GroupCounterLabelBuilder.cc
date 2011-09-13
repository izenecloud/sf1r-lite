#include "GroupCounterLabelBuilder.h"
#include "group_manager.h"
#include "StringGroupCounter.h"
#include "StringGroupLabel.h"
#include "NumericGroupCounter.h"
#include "NumericGroupLabel.h"
#include "NumericRangeGroupCounter.h"
#include "NumericRangeGroupLabel.h"
#include <configuration-manager/GroupConfig.h>
#include <search-manager/NumericPropertyTableBuilder.h>

#include <boost/lexical_cast.hpp>
#include <glog/logging.h>

NS_FACETED_BEGIN

GroupCounterLabelBuilder::GroupCounterLabelBuilder(
    const std::vector<GroupConfig>& groupConfigs,
    const GroupManager* groupManager,
    NumericPropertyTableBuilder* numericTableBuilder
)
    : groupConfigs_(groupConfigs)
    , groupManager_(groupManager)
    , numericTableBuilder_(numericTableBuilder)
{
}

PropertyDataType GroupCounterLabelBuilder::getPropertyType_(const std::string& prop) const
{
    for (std::vector<GroupConfig>::const_iterator it = groupConfigs_.begin();
        it != groupConfigs_.end(); ++it)
    {
        if (it->propName == prop)
            return it->propType;
    }

    return UNKNOWN_DATA_PROPERTY_TYPE;
}

const GroupConfig* GroupCounterLabelBuilder::getGroupConfig_(const std::string& prop) const
{
    for (std::vector<GroupConfig>::const_iterator it = groupConfigs_.begin();
        it != groupConfigs_.end(); ++it)
    {
        if (it->propName == prop)
            return &(*it);
    }

    return NULL;
}

GroupCounter* GroupCounterLabelBuilder::createGroupCounter(const GroupPropParam& groupPropParam)
{
    GroupCounter* counter = NULL;

    const std::string& propName = groupPropParam.property_;
    if (groupPropParam.isRange_)
    {
        counter = createNumericRangeCounter(propName);
    }
    else
    {
        counter = createValueCounter(propName);
    }

    return counter;
}

GroupCounter* GroupCounterLabelBuilder::createValueCounter(const std::string& prop) const
{
    GroupCounter* counter = NULL;

    PropertyDataType type = getPropertyType_(prop);
    switch(type)
    {
    case STRING_PROPERTY_TYPE:
        counter = createStringCounter(prop);
        break;

    case INT_PROPERTY_TYPE:
         counter = createNumericCounter<int64_t>(prop);
         break;

    case UNSIGNED_INT_PROPERTY_TYPE:
         counter = createNumericCounter<uint64_t>(prop);
         break;

    case FLOAT_PROPERTY_TYPE:
         counter = createNumericCounter<float>(prop);
         break;

    case DOUBLE_PROPERTY_TYPE:
         counter = createNumericCounter<double>(prop);
         break;

    default:
        LOG(ERROR) << "unsupported type " << type
                   << " for group property " << prop;
        break;
    }

    return counter;
}

GroupCounter* GroupCounterLabelBuilder::createStringCounter(const std::string& prop) const
{
    GroupCounter* counter = NULL;

    const PropValueTable* pvTable = groupManager_->getPropValueTable(prop);
    if (pvTable)
    {
        counter = new StringGroupCounter(*pvTable);
    }
    else
    {
        LOG(ERROR) << "group index file is not loaded for group property " << prop;
    }

    return counter;
}

template <typename T>
GroupCounter* GroupCounterLabelBuilder::createNumericCounter(const std::string& prop) const
{
    NumericPropertyTable *propertyTable = numericTableBuilder_->createPropertyTable(prop);

    if (propertyTable)
    {
        return new NumericGroupCounter<T>(propertyTable);
    }

    return NULL;
}


GroupCounter* GroupCounterLabelBuilder::createNumericRangeCounter(const std::string& prop) const
{
    const GroupConfig* groupConfig = getGroupConfig_(prop);
    if (!groupConfig || !groupConfig->isNumericType())
    {
        LOG(ERROR) << "property " << prop
                   << " must be configured as numeric type for range group";
        return NULL;
    }

    NumericPropertyTable *propertyTable = numericTableBuilder_->createPropertyTable(prop);
    if (propertyTable)
    {
        return new NumericRangeGroupCounter(propertyTable);
    }
    return NULL;
}

GroupLabel* GroupCounterLabelBuilder::createGroupLabel(const GroupParam::GroupLabel& labelParam)
{
    GroupLabel* label = NULL;

    const std::string& propName = labelParam.first;
    PropertyDataType type = getPropertyType_(propName);
    switch(type)
    {
    case STRING_PROPERTY_TYPE:
        label = createStringLabel(labelParam);
        break;

    case INT_PROPERTY_TYPE:
        label = createNumericLabel<int64_t>(labelParam);
        break;

    case UNSIGNED_INT_PROPERTY_TYPE:
        label = createNumericLabel<uint64_t>(labelParam);
        break;

    case FLOAT_PROPERTY_TYPE:
        label = createNumericLabel<float>(labelParam);
        break;

    case DOUBLE_PROPERTY_TYPE:
        label = createNumericLabel<double>(labelParam);
        break;

    default:
        LOG(ERROR) << "unsupported type " << type
                   << " for group property " << propName;
        break;
    }

    return label;
}

GroupLabel* GroupCounterLabelBuilder::createStringLabel(const GroupParam::GroupLabel& labelParam) const
{
    GroupLabel* label = NULL;

    const std::string& propName = labelParam.first;
    const PropValueTable* pvTable = groupManager_->getPropValueTable(propName);
    if (pvTable)
    {
        label = new StringGroupLabel(labelParam.second, *pvTable);
    }
    else
    {
        LOG(ERROR) << "group index file is not loaded for group property " << propName;
    }

    return label;
}

template <typename T>
GroupLabel* GroupCounterLabelBuilder::createNumericLabel(const GroupParam::GroupLabel& labelParam) const
{
    GroupLabel* label = NULL;

    if (!labelParam.second.empty())
    {
        const std::string& propValue = labelParam.second[0];
        try
        {
            T value = boost::lexical_cast<T>(propValue);
            const std::string& propName = labelParam.first;
            NumericPropertyTable *propertyTable = numericTableBuilder_->createPropertyTable(propName);
            if (propertyTable)
            {
                label = new NumericGroupLabel<T>(propertyTable, value);
            }
        }
        catch(const boost::bad_lexical_cast& e)
        {
            LOG(ERROR) << "failed in casting label from " << propValue
                       << " to numeric value, exception: " << e.what();
        }
    }

    return label;
}

NS_FACETED_END
