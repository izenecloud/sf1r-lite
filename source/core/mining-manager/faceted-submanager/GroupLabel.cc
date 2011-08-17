#include "GroupLabel.h"
#include <util/ustring/UString.h>

NS_FACETED_BEGIN

GroupLabel::GroupLabel(
    const std::pair<std::string, std::string>& label,
    const PropValueTable& pvTable,
    GroupCounter* counter
)
    : propValueTable_(pvTable)
    , targetValueId_(propValueTable_.propValueId(
        izenelib::util::UString(label.second, izenelib::util::UString::UTF_8)))
    , counter_(counter)
{
}

bool GroupLabel::test(docid_t doc) const
{
    return propValueTable_.testDoc(doc, targetValueId_);
}

NS_FACETED_END
