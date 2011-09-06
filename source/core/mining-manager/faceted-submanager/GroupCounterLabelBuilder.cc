#include "GroupCounterLabelBuilder.h"
#include "group_manager.h"
#include "StringGroupCounter.h"
#include "StringGroupLabel.h"
#include <configuration-manager/GroupConfig.h>

#include <glog/logging.h>

NS_FACETED_BEGIN

GroupCounterLabelBuilder::GroupCounterLabelBuilder(
    const std::vector<GroupConfig>& groupConfigs,
    const GroupManager* groupManager
)
    : groupConfigs_(groupConfigs)
    , groupManager_(groupManager)
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

GroupCounter* GroupCounterLabelBuilder::createGroupCounter(const std::string& prop) const
{
    GroupCounter* counter = NULL;

    PropertyDataType type = getPropertyType_(prop);
    switch(type)
    {
    case STRING_PROPERTY_TYPE:
        counter = createStringCounter(prop);
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

GroupLabel* GroupCounterLabelBuilder::createGroupLabel(const GroupParam::GroupLabel& labelParam) const
{
    GroupLabel* label = NULL;

    const std::string& propName = labelParam.first;
    PropertyDataType type = getPropertyType_(propName);
    switch(type)
    {
    case STRING_PROPERTY_TYPE:
        label = createStringLabel(labelParam);
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

NS_FACETED_END
