#include "GroupLabel.h"
#include <util/ustring/UString.h>

NS_FACETED_BEGIN

GroupLabel::GroupLabel(
    const std::pair<std::string, std::string>& label,
    const PropValueTable& pvTable,
    GroupCounter* counter
)
    : propValueTable_(pvTable)
    , valueIdTable_(pvTable.propIdTable())
    , parentIdTable_(pvTable.parentIdTable())
    , targetValueId_(propValueTable_.propValueId(
        izenelib::util::UString(label.second, izenelib::util::UString::UTF_8)))
    , counter_(counter)
{
}

bool GroupLabel::test(docid_t doc) const
{
    // this doc has no group index data yet
    if (doc >= valueIdTable_.size())
        return false;

    PropValueTable::pvid_t pvId = valueIdTable_[doc];

    // invalid id
    if (targetValueId_ == 0 || pvId == 0)
        return false;

    // pvId has no parent
    if (pvId >= parentIdTable_.size())
        return (pvId == targetValueId_);

    // whether targetValueId_ is pvId's parent
    while (pvId != 0)
    {
        if (pvId == targetValueId_)
            return true;

        pvId = parentIdTable_[pvId];
    }

    return false;
}

NS_FACETED_END
