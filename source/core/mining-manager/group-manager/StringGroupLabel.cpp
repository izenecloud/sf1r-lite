#include "StringGroupLabel.h"
#include <util/ustring/UString.h>

#include <set>

NS_FACETED_BEGIN

StringGroupLabel::StringGroupLabel(
    const GroupParam::GroupPathVec& labelPaths,
    const PropValueTable& pvTable
)
    : propValueTable_(pvTable)
    , lock_(pvTable.getMutex())
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
        for (std::vector<std::string>::const_iterator nodeIt = pathIt->begin();
            nodeIt != pathIt->end(); ++nodeIt)
        {
            ustrPath.push_back(izenelib::util::UString(*nodeIt, izenelib::util::UString::UTF_8));
        }

        PropValueTable::pvid_t targetId = propValueTable_.propValueId(ustrPath);
        targetValueIds_.push_back(targetId);
    }
}

NS_FACETED_END
