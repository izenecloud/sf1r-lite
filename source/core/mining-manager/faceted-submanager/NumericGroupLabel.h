///
/// @file NumericGroupLabel.h
/// @brief filter docs with selected label on numeric property
/// @author August Njam Grong <ran.long@izenesoft.com>
/// @date Created 2011-07-29
///

#ifndef SF1R_NUMERIC_GROUP_LABEL_H
#define SF1R_NUMERIC_GROUP_LABEL_H

#include <search-manager/NumericPropertyTable.h>
#include "GroupLabel.h"

#include <vector>
#include <string>

NS_FACETED_BEGIN

template<typename T>
class NumericGroupLabel : public GroupLabel
{
public:
    NumericGroupLabel(const NumericPropertyTable *propertyTable, const T &targetValue) : propertyTable_(propertyTable), targetValue_(targetValue)
    {}

    bool test(docid_t doc) const
    {
        T value;
        propertyTable_->getPropertyValue(doc, value);
        return (value == targetValue_);
    }

private:
    const NumericPropertyTable *propertyTable_;
    const T targetValue_;
};

NS_FACETED_END

#endif
