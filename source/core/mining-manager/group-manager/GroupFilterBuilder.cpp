#include "GroupFilterBuilder.h"
#include "GroupFilter.h"
#include "GroupParam.h"
#include "GroupCounterLabelBuilder.h"
#include "GroupManager.h"
#include "../attr-manager/AttrManager.h"
#include "../attr-manager/AttrTable.h"
#include <common/PropSharedLockSet.h>
#include <search-manager/NumericPropertyTableBuilder.h>

#include <memory> // auto_ptr
#include <glog/logging.h>

NS_FACETED_BEGIN

GroupFilterBuilder::GroupFilterBuilder(
        const GroupConfigMap& groupConfigMap,
        const GroupManager* groupManager,
        const AttrManager* attrManager,
        NumericPropertyTableBuilder* numericTableBuilder)
    : groupConfigMap_(groupConfigMap)
    , groupManager_(groupManager)
    , attrTable_(attrManager ? &attrManager->getAttrTable() : NULL)
    , numericTableBuilder_(numericTableBuilder)
{
}

GroupFilter* GroupFilterBuilder::createFilter(
        const GroupParam& groupParam,
        PropSharedLockSet& sharedLockSet) const
{
    if (groupParam.isEmpty())
        return NULL;

    std::auto_ptr<GroupFilter> groupFilter(new GroupFilter(groupParam));

    if (!groupParam.isGroupEmpty())
    {
        GroupCounterLabelBuilder builder(
            groupConfigMap_, groupManager_, numericTableBuilder_);
        if (!groupFilter->initGroup(builder, sharedLockSet))
            return NULL;
    }

    if (!groupParam.isAttrEmpty())
    {
        if (!attrTable_)
        {
            LOG(ERROR) << "attribute index file is not loaded";
            return NULL;
        }

        if (!groupFilter->initAttr(*attrTable_, sharedLockSet))
            return NULL;
    }

    return groupFilter.release();
}

NS_FACETED_END
