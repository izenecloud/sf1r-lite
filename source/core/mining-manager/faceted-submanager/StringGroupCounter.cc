#include "StringGroupCounter.h"
#include <util/ustring/UString.h>

NS_FACETED_BEGIN

StringGroupCounter::StringGroupCounter(const PropValueTable& pvTable)
    : propValueTable_(pvTable)
    , childMapTable_(pvTable.childMapTable())
    , countTable_(pvTable.propValueNum())
{
}

void StringGroupCounter::addDoc(docid_t doc)
{
    std::set<PropValueTable::pvid_t> parentSet;
    propValueTable_.parentIdSet(doc, parentSet);

    for (std::set<PropValueTable::pvid_t>::const_iterator it = parentSet.begin();
        it != parentSet.end(); ++it)
    {
        ++countTable_[*it];
    }

    // total doc count for this property
    if (!parentSet.empty())
    {
        ++countTable_[0];
    }
}

void StringGroupCounter::getGroupRep(GroupRep& groupRep)
{
    groupRep.stringGroupRep_.push_back(std::list<OntologyRepItem>());
    std::list<OntologyRepItem>& itemList = groupRep.stringGroupRep_.back();

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
    itemList.push_back(faceted::OntologyRepItem());
    faceted::OntologyRepItem& repItem = itemList.back();
    repItem.level = level;
    repItem.text = valueStr;
    repItem.doc_count = countTable_[pvId];

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
