///
/// @file NumericRangeGroupLabel.h
/// @brief filter docs with selected label on dynamically generated numeric range
/// @author August Njam Grong <ran.long@izenesoft.com>
/// @date Created 2011-09-07
///

#ifndef SF1R_NUMERIC_RANGE_GROUP_LABEL_H
#define SF1R_NUMERIC_RANGE_GROUP_LABEL_H

#include <search-manager/NumericPropertyTable.h>
#include "GroupLabel.h"

NS_FACETED_BEGIN

template <typename T>
class NumericRangeGroupLabel : public GroupLabel
{
public:
    NumericRangeGroupLabel(const NumericPropertyTable *propertyTable, const T &lowerBound, const T &upperBound)
        : propertyTable_(propertyTable)
        , lowerBound_(lowerBound)
        , upperBound_(upperBound)
    {}

    ~NumericRangeGroupLabel()
    {
        delete propertyTable_;
    }

    bool test(docid_t doc) const
    {
        T value;
        propertyTable_->getPropertyValue(doc, value);
        return (value >= lowerBound_ && value <= upperBound_);
    }

private:
    const NumericPropertyTable *propertyTable_;
    const T lowerBound_;
    const T upperBound_;
};

NS_FACETED_END

#endif
