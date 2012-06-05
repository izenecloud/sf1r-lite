#include "AttrLabel.h"
#include <util/ustring/UString.h>

#include <algorithm> // find

namespace
{
const izenelib::util::UString::EncodingType ENCODING_TYPE =
    izenelib::util::UString::UTF_8;
}

NS_FACETED_BEGIN

AttrLabel::AttrLabel(
    const AttrTable& attrTable,
    const std::string& attrName,
    const std::vector<std::string>& attrValues
)
    : attrTable_(attrTable)
    , lock_(attrTable.getMutex())
    , attrNameId_(attrTable.nameId(
        izenelib::util::UString(attrName, ENCODING_TYPE)))
{
    getTargetValueIds_(attrValues);
}

bool AttrLabel::test(docid_t doc) const
{
    AttrTable::ValueIdList valueIdList;
    attrTable_.getValueIdList(doc, valueIdList);

    for (std::size_t i = 0; i < valueIdList.size(); ++i)
    {
        if (targetValueIds_.find(valueIdList[i]) != targetIterEnd_)
            return true;
    }

    return false;
}

AttrTable::nid_t AttrLabel::attrNameId() const
{
    return attrNameId_;
}

void AttrLabel::getTargetValueIds_(const std::vector<std::string>& attrValues)
{
    for (std::vector<std::string>::const_iterator valueIt = attrValues.begin();
        valueIt != attrValues.end(); ++valueIt)
    {
        AttrTable::vid_t targetId = attrTable_.valueId(attrNameId_,
            izenelib::util::UString(*valueIt, ENCODING_TYPE));
        targetValueIds_.insert(targetId);
    }

    targetIterEnd_ = targetValueIds_.end();
}

NS_FACETED_END
