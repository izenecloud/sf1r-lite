///
/// @file GroupLabel.h
/// @brief base class to filter docs on selected property label
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-09-05
///

#ifndef SF1R_GROUP_LABEL_H
#define SF1R_GROUP_LABEL_H

#include "faceted_types.h"
#include "GroupCounter.h"

NS_FACETED_BEGIN

class GroupLabel
{
public:
    GroupLabel() {}

    void setCounter(GroupCounter* counter) {
        counter_ = counter;
    }

    GroupCounter* getCounter() {
        return counter_;
    }

    virtual bool test(docid_t doc) const = 0;

private:
    GroupCounter* counter_;
};

NS_FACETED_END

#endif 
