///
/// @file NumericRangeGroupCounter.h
/// @brief count docs for numeric property and dynamically split them into ranges
/// @author August Njam Grong <ran.long@izenesoft.com>
/// @date Created 2011-09-07
///

#ifndef SF1R_NUMERIC_RANGE_GROUP_COUNTER_H
#define SF1R_NUMERIC_RANGE_GROUP_COUNTER_H

#include "GroupCounter.h"

namespace sf1r{
class NumericPropertyTable;
}

NS_FACETED_BEGIN

struct Log10SegmentTree
{
    Log10SegmentTree();

    void insertPoint(int64_t point);

    size_t level0_;
    size_t level1_[8];
    size_t level2_[8][10];
    size_t level3_[8][10][10];
};

class NumericRangeGroupCounter : public GroupCounter
{
public:
    NumericRangeGroupCounter(const NumericPropertyTable *propertyTable, size_t rangeCount = 5);

    ~NumericRangeGroupCounter();

    virtual void addDoc(docid_t doc);

    virtual void getGroupRep(OntologyRep& groupRep) const;

private:
    const NumericPropertyTable *propertyTable_;
    size_t rangeCount_;
    Log10SegmentTree log10SegmentTree_;
};

NS_FACETED_END

#endif
