#include "AttrLabel.h"
#include <util/ustring/UString.h>

#include <algorithm> // find

NS_FACETED_BEGIN

AttrLabel::AttrLabel(
    const std::pair<std::string, std::string>& label,
    const AttrTable* attrTable
)
    : valueIdTable_(attrTable->valueIdTable())
    , attrNameId_(attrTable->nameId(
        izenelib::util::UString(label.first, izenelib::util::UString::UTF_8)))
    , targetValueId_(attrTable->valueId(
        attrNameId_,
        izenelib::util::UString(label.second, izenelib::util::UString::UTF_8)))
{
}

bool AttrLabel::test(docid_t doc) const
{
    // this doc has no attr index data yet
    if (doc >= valueIdTable_.size())
        return false;

    const AttrTable::ValueIdList& valueIdList = valueIdTable_[doc];

    AttrTable::ValueIdList::const_iterator findIt =
        std::find(valueIdList.begin(), valueIdList.end(), targetValueId_);

    return (findIt != valueIdList.end());
}

AttrTable::nid_t AttrLabel::attrNameId() const
{
    return attrNameId_;
}

NS_FACETED_END
