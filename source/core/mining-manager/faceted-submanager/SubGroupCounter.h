///
/// @file SubGroupCounter.h
/// @brief for each property value, besides a count value,
///        in order to get 2nd level group results for sub-property,
///        it also stores a pointer to the sub-property GroupCounter.
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2012-01-04
///

#ifndef SF1R_SUB_GROUP_COUNTER_H
#define SF1R_SUB_GROUP_COUNTER_H

#include "faceted_types.h"
#include "GroupCounter.h"

#include <boost/scoped_ptr.hpp>

NS_FACETED_BEGIN

struct SubGroupCounter
{
    unsigned int count_;
    boost::scoped_ptr<GroupCounter> groupCounter_;

    SubGroupCounter(GroupCounter* groupCounter)
        : count_(0)
        , groupCounter_(groupCounter->clone())
    {}

    SubGroupCounter(const SubGroupCounter& subGroupCounter)
        : count_(subGroupCounter.count_)
        , groupCounter_(subGroupCounter.groupCounter_->clone())
    {}
};

NS_FACETED_END

#endif 
