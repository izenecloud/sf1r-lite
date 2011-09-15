///
/// @file NumericRangeGroupCounter.h
/// @brief count docs for numeric property and dynamically split them into ranges
/// @author August Njam Grong <ran.long@izenesoft.com>
/// @date Created 2011-09-07
///

#ifndef SF1R_NUMERIC_RANGE_GROUP_COUNTER_H
#define SF1R_NUMERIC_RANGE_GROUP_COUNTER_H

#include "GroupCounter.h"

#define LEVEL_1_OF_SEGMENT_TREE 7

namespace sf1r{
class NumericPropertyTable;
}

NS_FACETED_BEGIN

struct Log10SegmentTree
{
    Log10SegmentTree();

    void insertPoint(int64_t point);

    int level0_;
    int level1_[LEVEL_1_OF_SEGMENT_TREE];
    int level2_[LEVEL_1_OF_SEGMENT_TREE][10];
    int level3_[LEVEL_1_OF_SEGMENT_TREE][10][10];
    static const int bound_[LEVEL_1_OF_SEGMENT_TREE + 1];
};

class NumericRangeGroupCounter : public GroupCounter
{
public:
    enum RangePolicy {
        RAW,
        DROP_BLANKS,
        SPLIT,
        SPLIT_AND_MERGE
    };

public:
    NumericRangeGroupCounter(const NumericPropertyTable *propertyTable, RangePolicy rangePolicy = SPLIT, int maxRangeNumber = 7);

    ~NumericRangeGroupCounter();

    virtual void addDoc(docid_t doc);

    virtual void getGroupRep(OntologyRep& groupRep) const;

private:
    void get_range_list_raw(std::list<OntologyRepItem>& itemList) const;

    void get_range_list_drop_blanks(std::list<OntologyRepItem>& itemList) const;

    void get_range_list_split(std::list<OntologyRepItem>& itemList) const;

    void get_range_list_split_and_merge(std::list<OntologyRepItem>& itemList) const;

private:
    const NumericPropertyTable *propertyTable_;
    RangePolicy rangePolicy_;
    int maxRangeNumber_;
    Log10SegmentTree segmentTree_;
};

NS_FACETED_END

#endif
