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

class DecimalSegmentTreeNode
{
public:
    DecimalSegmentTreeNode(int start, size_t span, DecimalSegmentTreeNode* parent = NULL);

    ~DecimalSegmentTreeNode();

    void clear();

    int getStart() const;

    size_t getSpan() const;

    size_t getCount() const;

    void insertPoint(unsigned char *digits, size_t level);

private:
    int start_;
    size_t span_;
    size_t count_;
    DecimalSegmentTreeNode* children_[10];
    DecimalSegmentTreeNode* parent_;
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
    DecimalSegmentTreeNode decimalSegmentTree_;
};

NS_FACETED_END

#endif
