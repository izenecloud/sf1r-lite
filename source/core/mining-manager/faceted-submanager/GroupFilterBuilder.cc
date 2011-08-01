#include "GroupFilterBuilder.h"
#include "GroupFilter.h"
#include "GroupParam.h"

#include "attr_manager.h"
#include "attr_table.h"

namespace
{
const sf1r::faceted::GroupManager::PropValueMap EMPTY_PROP_VALUE_MAP;
const sf1r::faceted::AttrTable EMPTY_ATTR_TABLE;
}

NS_FACETED_BEGIN

GroupFilterBuilder::GroupFilterBuilder(
    const GroupManager* groupManager,
    const AttrManager* attrManager
)
    : propValueMap_(groupManager ? groupManager->getPropValueMap() : EMPTY_PROP_VALUE_MAP)
    , attrTable_(attrManager ? attrManager->getAttrTable() : EMPTY_ATTR_TABLE)
{
}

GroupFilter* GroupFilterBuilder::createFilter(const GroupParam& groupParam) const
{
    if (groupParam.empty())
        return NULL;

    return new GroupFilter(propValueMap_, attrTable_, groupParam);
}

NS_FACETED_END
