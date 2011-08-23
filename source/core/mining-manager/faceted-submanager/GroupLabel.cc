#include "GroupLabel.h"
#include "GroupCounter.h"
#include <util/ustring/UString.h>

NS_FACETED_BEGIN

GroupLabel::GroupLabel(
    const std::vector<std::string>& labelPath,
    const PropValueTable& pvTable,
    GroupCounter* counter
)
    : propValueTable_(pvTable)
    , targetValueId_(0)
    , counter_(counter)
{
    std::vector<izenelib::util::UString> ustrPath;
    for (std::vector<std::string>::const_iterator it = labelPath.begin();
        it != labelPath.end(); ++it)
    {
        ustrPath.push_back(izenelib::util::UString(*it, izenelib::util::UString::UTF_8));
    }
    targetValueId_ = propValueTable_.propValueId(ustrPath);
}

bool GroupLabel::test(docid_t doc) const
{
    return propValueTable_.testDoc(doc, targetValueId_);
}

NS_FACETED_END
