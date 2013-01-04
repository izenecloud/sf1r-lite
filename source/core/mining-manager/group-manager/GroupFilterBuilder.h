///
/// @file GroupFilterBuilder.h
/// @brief create instance of GroupFilter
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-07-28
///

#ifndef SF1R_GROUP_FILTER_BUILDER_H
#define SF1R_GROUP_FILTER_BUILDER_H

#include "../faceted-submanager/faceted_types.h"
#include <configuration-manager/GroupConfig.h>

#include <vector>

namespace sf1r
{
class NumericPropertyTableBuilder;
class PropSharedLockSet;
}

NS_FACETED_BEGIN

class GroupManager;
class AttrManager;
class GroupFilter;
class AttrTable;
class GroupParam;

class GroupFilterBuilder
{
public:
    GroupFilterBuilder(
        const GroupConfigMap& groupConfigMap,
        const GroupManager* groupManager,
        const AttrManager* attrManager,
        NumericPropertyTableBuilder* numericTableBuilder);

    /**
     * create group filter instance.
     * @param groupParam group parameter
     * @return group filter instance,
     *         if @p groupParam.empty() is true, that is, it's not necessary
     *         to do group filter, NULL is returned.
     */
    GroupFilter* createFilter(
        const GroupParam& groupParam,
        PropSharedLockSet& sharedLockSet) const;

private:
    const GroupConfigMap& groupConfigMap_;
    const GroupManager* groupManager_;
    const AttrTable* attrTable_;
    NumericPropertyTableBuilder* numericTableBuilder_;
};

NS_FACETED_END

#endif
