///
/// @file NumericRangeGroupCounter.h
/// @brief count docs for numeric property and dynamically split them into ranges
/// @author August Njam Grong <ran.long@izenesoft.com>
/// @date Created 2011-09-07
///

#ifndef SF1R_NUMERIC_RANGE_GROUP_COUNTER_H
#define SF1R_NUMERIC_RANGE_GROUP_COUNTER_H

#include "GroupCounter.h"

#include <common/NumericPropertyTableBase.h>

#define LEVEL_1_OF_SEGMENT_TREE 7
#define MAX_RANGE_NUMBER 7

NS_FACETED_BEGIN

class NumericRangeGroupCounter : public GroupCounter
{
public:
    NumericRangeGroupCounter(const std::string& property, const NumericPropertyTableBase* numericPropertyTable);

    ~NumericRangeGroupCounter();

    virtual NumericRangeGroupCounter* clone() const;

    virtual void addDoc(docid_t doc);

    virtual void getGroupRep(GroupRep &groupRep);

    static void toOntologyRepItemList(GroupRep &groupRep);

private:
    const std::string property_;
    const NumericPropertyTableBase* numericPropertyTable_;
    std::vector<unsigned int> segmentTree_;

    static const int bound_[LEVEL_1_OF_SEGMENT_TREE + 1];
};

NS_FACETED_END

#endif
