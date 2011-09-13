///
/// @file NumericRangeGroupLabel.h
/// @brief filter docs with selected label on dynamically generated numeric range
/// @author August Njam Grong <ran.long@izenesoft.com>
/// @date Created 2011-09-07
///

#ifndef SF1R_NUMERIC_RANGE_GROUP_LABEL_H
#define SF1R_NUMERIC_RANGE_GROUP_LABEL_H

#include "GroupLabel.h"

namespace sf1r{
class NumericPropertyTable;
}

NS_FACETED_BEGIN

class NumericRangeGroupLabel : public GroupLabel
{
public:
    NumericRangeGroupLabel(const NumericPropertyTable *propertyTable, const int64_t &lowerBound);

    NumericRangeGroupLabel(const NumericPropertyTable *propertyTable, const int64_t &lowerBound, const int64_t &upperBound);

    ~NumericRangeGroupLabel();

    bool test(docid_t doc) const;

private:
    bool test1(docid_t doc) const;
    bool test2(docid_t doc) const;

private:
    const NumericPropertyTable *propertyTable_;
    const int64_t lowerBound_;
    const int64_t upperBound_;
    bool (NumericRangeGroupLabel::*test_)(docid_t doc) const;
};

NS_FACETED_END

#endif
