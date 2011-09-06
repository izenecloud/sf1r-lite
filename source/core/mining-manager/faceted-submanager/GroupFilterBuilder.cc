#include "GroupFilterBuilder.h"
#include "GroupFilter.h"
#include "GroupParam.h"
#include "GroupCounterLabelBuilder.h"
#include "group_manager.h"
#include "attr_manager.h"
#include "attr_table.h"
#include <configuration-manager/GroupConfig.h>

#include <memory> // auto_ptr
#include <glog/logging.h>

NS_FACETED_BEGIN

GroupFilterBuilder::GroupFilterBuilder(
    const std::vector<GroupConfig>& groupConfigs,
    const GroupManager* groupManager,
    const AttrManager* attrManager
)
    : groupConfigs_(groupConfigs)
    , groupManager_(groupManager)
    , attrTable_(attrManager ? attrManager->getAttrTable() : NULL)
{
}

GroupFilter* GroupFilterBuilder::createFilter(const GroupParam& groupParam) const
{
    if (groupParam.empty())
        return NULL;

    std::auto_ptr<GroupFilter> groupFilter(new GroupFilter(groupParam));

    if (!groupParam.groupProps_.empty())
    {
        GroupCounterLabelBuilder builder(groupConfigs_, groupManager_);
        if (!groupFilter->initGroup(builder))
            return NULL;
    }

    if (groupParam.isAttrGroup_)
    {
        if (!attrTable_)
        {
            LOG(ERROR) << "attribute index file is not loaded";
            return NULL;
        }

        if (!groupFilter->initAttr(attrTable_))
            return NULL;
    }

    return groupFilter.release();
}

NS_FACETED_END
