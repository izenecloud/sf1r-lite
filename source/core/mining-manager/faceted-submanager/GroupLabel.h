///
/// @file GroupLabel.h
/// @brief filter docs on selected property label
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-07-29
///

#ifndef SF1R_GROUP_LABEL_H
#define SF1R_GROUP_LABEL_H

#include "faceted_types.h"
#include "prop_value_table.h"
#include "GroupCounter.h"

NS_FACETED_BEGIN

class GroupLabel
{
public:
    GroupLabel(
        const std::pair<std::string, std::string>& label,
        const PropValueTable& pvTable,
        GroupCounter* counter
    );

    bool test(docid_t doc) const;

    GroupCounter* getCounter() {
        return counter_;
    }

private:
    const PropValueTable& propValueTable_;
    const PropValueTable::pvid_t targetValueId_;
    GroupCounter* counter_;
};

NS_FACETED_END

#endif 
