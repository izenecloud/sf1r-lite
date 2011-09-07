///
/// @file GroupCounterLabelBuilder.h
/// @brief create instance of GroupFilter
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-09-05
///

#ifndef SF1R_GROUP_COUNTER_LABEL_BUILDER_H
#define SF1R_GROUP_COUNTER_LABEL_BUILDER_H

#include "GroupParam.h"
#include <common/type_defs.h> // PropertyDataType

#include <vector>
#include <string>

namespace sf1r
{
class GroupConfig;
class NumericPropertyTableBuilder;
}

NS_FACETED_BEGIN

class GroupManager;
class GroupCounter;
class GroupLabel;

class GroupCounterLabelBuilder
{
public:
    GroupCounterLabelBuilder(
        const std::vector<GroupConfig>& groupConfigs,
        const GroupManager* groupManager,
        NumericPropertyTableBuilder* numericTableBuilder
    );

    GroupCounter* createGroupCounter(const std::string& prop);
    GroupLabel* createGroupLabel(const GroupParam::GroupLabel& labelParam);

private:
    PropertyDataType getPropertyType_(const std::string& prop) const;

    GroupCounter* createStringCounter(const std::string& prop);
    GroupLabel* createStringLabel(const GroupParam::GroupLabel& labelParam);

    template <typename T>
    GroupLabel* createNumericLabel(const GroupParam::GroupLabel& labelParam) const;

private:
    const std::vector<GroupConfig>& groupConfigs_;
    const GroupManager* groupManager_;
    NumericPropertyTableBuilder* numericTableBuilder_;
};

NS_FACETED_END

#endif 
