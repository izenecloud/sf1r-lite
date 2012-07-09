///
/// @file DateGroupLabel.h
/// @brief filter docs with selected label on date property
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2012-07-04
///

#ifndef SF1R_DATE_GROUP_LABEL_H
#define SF1R_DATE_GROUP_LABEL_H

#include "GroupLabel.h"
#include "DateGroupTable.h"
#include "DateStrParser.h"
#include "GroupParam.h"

#include <vector>

NS_FACETED_BEGIN

class DateGroupLabel : public GroupLabel
{
public:
    typedef std::vector<DateStrParser::DateMask> DateMaskVec;

    DateGroupLabel(
        const DateGroupTable& dateTable,
        const DateMaskVec& dateMaskVec
    );

    virtual bool test(docid_t doc) const;

    virtual void insertSharedLock(SharedLockSet& lockSet) const
    {
        lockSet.insert(&dateTable_);
    }

private:
    const DateGroupTable& dateTable_;

    const DateMaskVec targetDateMasks_;

    mutable DateGroupTable::DateSet dateSet_;
};

NS_FACETED_END

#endif // SF1R_DATE_GROUP_LABEL_H
