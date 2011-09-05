#include "GroupCounterLabelBuilder.h"
#include "group_manager.h"
#include "StringGroupCounter.h"
#include "StringGroupLabel.h"

#include <glog/logging.h>

NS_FACETED_BEGIN

GroupCounterLabelBuilder::GroupCounterLabelBuilder(const GroupManager* groupManager)
    : groupManager_(groupManager)
{
}

GroupCounter* GroupCounterLabelBuilder::createGroupCounter(const std::string& prop) const
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
    const PropValueTable* pvTable = groupManager_->getPropValueTable(labelParam.first);
    if (pvTable)
    {
        label = new StringGroupLabel(labelParam.second, *pvTable);
    }
    else
    {
        LOG(ERROR) << "group index file is not loaded for group property " << labelParam.first;
    }

    return label;
}

NS_FACETED_END
