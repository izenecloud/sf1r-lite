#include "AttrLabel.h"
#include <util/ustring/UString.h>

#include <algorithm> // find

NS_FACETED_BEGIN

AttrLabel::AttrLabel(
    const std::pair<std::string, std::string>& label,
    const AttrTable& attrTable
)
    : attrTable_(attrTable)
    , lock_(attrTable.getMutex())
    , attrNameId_(attrTable.nameId(
        izenelib::util::UString(label.first, izenelib::util::UString::UTF_8)))
    , targetValueId_(attrTable.valueId(
        attrNameId_,
        izenelib::util::UString(label.second, izenelib::util::UString::UTF_8)))
{
}

bool AttrLabel::test(docid_t doc) const
{
    AttrTable::ValueIdList valueIdList;
    attrTable_.getValueIdList(doc, valueIdList);

    for (std::size_t i = 0; i < valueIdList.size(); ++i)
    {
        if (valueIdList[i] == targetValueId_)
            return true;
    }

    return false;
}

AttrTable::nid_t AttrLabel::attrNameId() const
{
    return attrNameId_;
}

NS_FACETED_END
