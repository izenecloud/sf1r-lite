#include "StringGroupLabel.h"
#include "../util/split_ustr.h"
#include <util/ustring/UString.h>

#include <set>

NS_FACETED_BEGIN

StringGroupLabel::StringGroupLabel(
    const GroupParam::GroupPathVec& labelPaths,
    const PropValueTable& pvTable
)
    : propValueTable_(pvTable)
    , alloc_(recycle_)
    , parentSet_(std::less<PropValueTable::pvid_t>(), alloc_)
{
    getTargetValueIds_(labelPaths);
}

bool StringGroupLabel::test(docid_t doc) const
{
    parentSet_.clear();
    propValueTable_.parentIdSet(doc, parentSet_);
    PropValueTable::ParentSetType::const_iterator parentEndIt = parentSet_.end();

    for (std::vector<PropValueTable::pvid_t>::const_iterator it = targetValueIds_.begin();
        it != targetValueIds_.end(); ++it)
    {
        if (parentSet_.find(*it) != parentEndIt)
            return true;
    }

    return false;
}

void StringGroupLabel::getTargetValueIds_(const GroupParam::GroupPathVec& labelPaths)
{
    for (GroupParam::GroupPathVec::const_iterator pathIt = labelPaths.begin();
        pathIt != labelPaths.end(); ++pathIt)
    {
        std::vector<izenelib::util::UString> ustrPath;
        convert_to_ustr_path(*pathIt, ustrPath);

        PropValueTable::pvid_t targetId =
            propValueTable_.propValueId(ustrPath, false);
        targetValueIds_.push_back(targetId);
    }
}

NS_FACETED_END
