///
/// @file StringGroupLabel.h
/// @brief filter docs with selected label on string property
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-07-29
///

#ifndef SF1R_STRING_GROUP_LABEL_H
#define SF1R_STRING_GROUP_LABEL_H

#include "GroupLabel.h"
#include "prop_value_table.h"

#include <vector>
#include <string>

NS_FACETED_BEGIN

class StringGroupLabel : public GroupLabel
{
public:
    StringGroupLabel(
        const std::vector<std::string>& labelPath,
        const PropValueTable& pvTable
    );

    bool test(docid_t doc) const;

private:
    const PropValueTable& propValueTable_;
    PropValueTable::pvid_t targetValueId_;
};

NS_FACETED_END

#endif 
