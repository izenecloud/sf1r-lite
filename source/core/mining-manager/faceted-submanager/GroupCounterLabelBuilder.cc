#include "GroupCounterLabelBuilder.h"
#include "group_manager.h"
#include "StringGroupCounter.h"
#include "StringGroupLabel.h"
#include "NumericGroupCounter.h"
#include "NumericGroupLabel.h"
#include <configuration-manager/GroupConfig.h>
#include <search-manager/SearchManager.h>

#include <boost/lexical_cast.hpp>
#include <glog/logging.h>

NS_FACETED_BEGIN

GroupCounterLabelBuilder::GroupCounterLabelBuilder(
    const std::vector<GroupConfig>& groupConfigs,
    const GroupManager* groupManager,
    const SearchManager* searchManager
)
    : groupConfigs_(groupConfigs)
    , groupManager_(groupManager)
    , searchManager_(searchManager)
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

    case INT_PROPERTY_TYPE:
        counter = new NumericGroupCounter<int64_t>(searchManager_->createPropertyTable(prop));
        break;

    case UNSIGNED_INT_PROPERTY_TYPE:
        counter = new NumericGroupCounter<uint64_t>(searchManager_->createPropertyTable(prop));
        break;

    case FLOAT_PROPERTY_TYPE:
        counter = new NumericGroupCounter<float>(searchManager_->createPropertyTable(prop));
        break;

    case DOUBLE_PROPERTY_TYPE:
        counter = new NumericGroupCounter<double>(searchManager_->createPropertyTable(prop));
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

    case INT_PROPERTY_TYPE:
        label = new NumericGroupLabel<int64_t>(searchManager_->createPropertyTable(propName), boost::lexical_cast<int64_t>(labelParam.second[0]));
        break;

    case UNSIGNED_INT_PROPERTY_TYPE:
        label = new NumericGroupLabel<uint64_t>(searchManager_->createPropertyTable(propName), boost::lexical_cast<uint64_t>(labelParam.second[0]));
        break;

    case FLOAT_PROPERTY_TYPE:
        label = new NumericGroupLabel<float>(searchManager_->createPropertyTable(propName), boost::lexical_cast<float>(labelParam.second[0]));
        break;

    case DOUBLE_PROPERTY_TYPE:
        label = new NumericGroupLabel<double>(searchManager_->createPropertyTable(propName), boost::lexical_cast<double>(labelParam.second[0]));
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
