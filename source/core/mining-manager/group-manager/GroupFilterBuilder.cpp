#include "GroupFilterBuilder.h"
#include "GroupFilter.h"
#include "GroupParam.h"
#include "GroupCounterLabelBuilder.h"
#include "GroupManager.h"
#include "../attr-manager/AttrManager.h"
#include "../attr-manager/AttrTable.h"
#include <configuration-manager/GroupConfig.h>
#include <search-manager/NumericPropertyTableBuilder.h>

#include <memory> // auto_ptr
#include <glog/logging.h>

NS_FACETED_BEGIN

GroupFilterBuilder::GroupFilterBuilder(
    const std::vector<GroupConfig>& groupConfigs,
    const GroupManager* groupManager,
    const AttrManager* attrManager,
    NumericPropertyTableBuilder* numericTableBuilder
)
    : groupConfigs_(groupConfigs)
    , groupManager_(groupManager)
    , attrTable_(attrManager ? &attrManager->getAttrTable() : NULL)
    , numericTableBuilder_(numericTableBuilder)
{
}

GroupFilter* GroupFilterBuilder::createFilter(const GroupParam& groupParam) const
{
    if (groupParam.isEmpty())
        return NULL;

    std::auto_ptr<GroupFilter> groupFilter(new GroupFilter(groupParam));

    if (!groupParam.isGroupEmpty())
    {
        GroupCounterLabelBuilder builder(groupConfigs_, groupManager_, numericTableBuilder_);
        if (!groupFilter->initGroup(builder))
            return NULL;
    }

    if (!groupParam.isAttrEmpty())
    {
        if (!attrTable_)
        {
            LOG(ERROR) << "attribute index file is not loaded";
            return NULL;
        }

        if (!groupFilter->initAttr(*attrTable_))
            return NULL;
    }

    return groupFilter.release();
}

NS_FACETED_END
