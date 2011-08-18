#include "GroupFilterBuilder.h"
#include "GroupFilter.h"
#include "GroupParam.h"

#include "group_manager.h"
#include "attr_manager.h"
#include "attr_table.h"

namespace
{
const sf1r::faceted::AttrTable EMPTY_ATTR_TABLE;
}

NS_FACETED_BEGIN

GroupFilterBuilder::GroupFilterBuilder(
    const GroupManager* groupManager,
    const AttrManager* attrManager
)
    : groupManager_(groupManager)
    , attrTable_(attrManager ? attrManager->getAttrTable() : EMPTY_ATTR_TABLE)
{
}

GroupFilter* GroupFilterBuilder::createFilter(const GroupParam& groupParam) const
{
    if (groupParam.empty())
        return NULL;

    return new GroupFilter(groupManager_, attrTable_, groupParam);
}

NS_FACETED_END
