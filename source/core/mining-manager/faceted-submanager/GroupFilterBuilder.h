///
/// @file GroupFilterBuilder.h
/// @brief create instance of GroupFilter
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-07-28
///

#ifndef SF1R_GROUP_FILTER_BUILDER_H
#define SF1R_GROUP_FILTER_BUILDER_H

#include "faceted_types.h"

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
        const GroupManager* groupManager,
        const AttrManager* attrManager
    );

    /**
     * create group filter instance.
     * @param groupParam group parameter
     * @return group filter instance,
     *         if @p groupParam.empty() is true, that is, it's not necessary
     *         to do group filter, NULL is returned.
     */
    GroupFilter* createFilter(const GroupParam& groupParam) const;

private:
    const GroupManager* groupManager_;
    const AttrTable& attrTable_;
};

NS_FACETED_END

#endif 
