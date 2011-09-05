///
/// @file GroupCounterLabelBuilder.h
/// @brief create instance of GroupFilter
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-09-05
///

#ifndef SF1R_GROUP_COUNTER_LABEL_BUILDER_H
#define SF1R_GROUP_COUNTER_LABEL_BUILDER_H

#include "GroupParam.h"
#include <configuration-manager/PropertyConfig.h>

#include <string>

NS_FACETED_BEGIN

class GroupManager;
class GroupCounter;
class GroupLabel;

class GroupCounterLabelBuilder
{
public:
    GroupCounterLabelBuilder(
        const schema_type& schema,
        const GroupManager* groupManager
    );

    GroupCounter* createGroupCounter(const std::string& prop) const;
    GroupLabel* createGroupLabel(const GroupParam::GroupLabel& labelParam) const;

private:
    PropertyDataType getPropertyType_(const std::string& prop) const;

    GroupCounter* createStringCounter(const std::string& prop) const;
    GroupLabel* createStringLabel(const GroupParam::GroupLabel& labelParam) const;

private:
    const schema_type& schema_;
    const GroupManager* groupManager_;
};

NS_FACETED_END

#endif 
