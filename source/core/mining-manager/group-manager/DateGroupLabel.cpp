#include "DateGroupLabel.h"

NS_FACETED_BEGIN

DateGroupLabel::DateGroupLabel(
    const DateGroupTable& dateTable,
    const DateMaskVec& dateMaskVec
)
    : dateTable_(dateTable)
    , targetDateMasks_(dateMaskVec)
{
}

bool DateGroupLabel::test(docid_t doc) const
{
    for (DateMaskVec::const_iterator it = targetDateMasks_.begin();
        it != targetDateMasks_.end(); ++it)
    {
        dateSet_.clear();
        dateTable_.getDateSet(doc, it->mask_, dateSet_);

        if (dateSet_.find(it->date_) != dateSet_.end())
            return true;
    }

    return false;
}

NS_FACETED_END
