///
/// @file NumericRangeGroupLabel.h
/// @brief filter docs with selected label on dynamically generated numeric range
/// @author August Njam Grong <ran.long@izenesoft.com>
/// @date Created 2011-09-07
///

#ifndef SF1R_NUMERIC_RANGE_GROUP_LABEL_H
#define SF1R_NUMERIC_RANGE_GROUP_LABEL_H

#include "GroupLabel.h"

#include <vector>
#include <set>
#include <utility> // std::pair

namespace sf1r{
class NumericPropertyTable;
}

NS_FACETED_BEGIN

class NumericRangeGroupLabel : public GroupLabel
{
public:
    /** lower bound and upper bound */
    typedef std::pair<int64_t, int64_t> NumericRange;
    typedef std::vector<NumericRange> NumericRangeVec;

    NumericRangeGroupLabel(const NumericPropertyTable *propertyTable, const std::vector<float>& targetValues);
    NumericRangeGroupLabel(const NumericPropertyTable *propertyTable, const NumericRangeVec& ranges);

    ~NumericRangeGroupLabel();

    bool test(docid_t doc) const;

private:
    bool test1(docid_t doc) const;
    bool test2(docid_t doc) const;

private:
    const NumericPropertyTable *propertyTable_;
    NumericRangeVec ranges_;
    std::set<float> targetValueSet_;
    bool (NumericRangeGroupLabel::*test_)(docid_t doc) const;
};

NS_FACETED_END

#endif
