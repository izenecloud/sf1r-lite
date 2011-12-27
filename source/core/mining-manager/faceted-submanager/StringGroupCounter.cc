#include "StringGroupCounter.h"

#include <util/ustring/UString.h>
#include "GroupRep.h"

NS_FACETED_BEGIN

StringGroupCounter::StringGroupCounter(const PropValueTable& pvTable)
    : propValueTable_(pvTable)
    , childMapTable_(pvTable.childMapTable())
    , countTable_(pvTable.propValueNum())
    //, alloc_(recycle_)
    , parentSet_(std::less<PropValueTable::pvid_t>())
{
}

void StringGroupCounter::addDoc(docid_t doc)
{
    parentSet_.clear();
    propValueTable_.parentIdSet(doc, parentSet_);

    for (PropValueTable::ParentSetType::const_iterator it = parentSet_.begin();
        it != parentSet_.end(); ++it)
    {
        ++countTable_[*it];
    }

    // total doc count for this property
    if (!parentSet_.empty())
    {
        ++countTable_[0];
    }
}

void StringGroupCounter::getGroupRep(GroupRep& groupRep)
{
    GroupRep::StringGroupRep& itemList = groupRep.stringGroupRep_;

    izenelib::util::UString propName(propValueTable_.propName(), UString::UTF_8);
    // start from id 0 at level 0
    appendGroupRep(itemList, 0, 0, propName);
}

void StringGroupCounter::appendGroupRep(
    std::list<OntologyRepItem>& itemList,
    PropValueTable::pvid_t pvId,
    int level,
    const izenelib::util::UString& valueStr
) const
{
    itemList.push_back(faceted::OntologyRepItem(level, valueStr, 0, countTable_[pvId]));

    const PropValueTable::PropStrMap& propStrMap = childMapTable_[pvId];
    for (PropValueTable::PropStrMap::const_iterator it = propStrMap.begin();
        it != propStrMap.end(); ++it)
    {
        PropValueTable::pvid_t childId = it->second;
        if (countTable_[childId])
        {
            appendGroupRep(itemList, childId, level+1, it->first);
        }
    }
}

NS_FACETED_END
