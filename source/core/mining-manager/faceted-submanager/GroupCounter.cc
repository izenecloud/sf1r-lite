#include "GroupCounter.h"
#include <util/ustring/UString.h>

NS_FACETED_BEGIN

GroupCounter::GroupCounter(const PropValueTable& pvTable)
    : propValueTable_(pvTable)
    , countTable_(pvTable.propValueNum())
{
}

void GroupCounter::addDoc(docid_t doc)
{
    std::set<PropValueTable::pvid_t> parentSet;
    propValueTable_.parentIdSet(doc, parentSet);

    for (std::set<PropValueTable::pvid_t>::const_iterator it = parentSet.begin();
        it != parentSet.end(); ++it)
    {
        ++countTable_[*it];
    }
}

void GroupCounter::getGroupRep(OntologyRep& groupRep)
{
    std::list<OntologyRepItem>& itemList = groupRep.item_list;

    // property name as root node
    itemList.push_back(OntologyRepItem());
    OntologyRepItem& propItem = itemList.back();
    propItem.text.assign(propValueTable_.propName(),
                         izenelib::util::UString::UTF_8);

    // nodes at configured level
    const std::list<OntologyRepItem>& treeList = propValueTable_.valueTree().item_list;
    for (std::list<OntologyRepItem>::const_iterator it = treeList.begin();
        it != treeList.end(); ++it)
    {
        int count = countTable_[it->id];
        if (count)
        {
            itemList.push_back(*it);
            OntologyRepItem& valueItem = itemList.back();
            valueItem.doc_count = count;

            if (valueItem.level == 1)
            {
                propItem.doc_count += count;
            }

            // clear configured node
            countTable_[it->id] = 0;
        }
    }

    // other nodes are appended as level 1
    for (std::size_t valueId = 1; valueId < countTable_.size(); ++valueId)
    {
        int count = countTable_[valueId];
        if (count)
        {
            itemList.push_back(sf1r::faceted::OntologyRepItem());
            sf1r::faceted::OntologyRepItem& valueItem = itemList.back();
            valueItem.level = 1;
            valueItem.text = propValueTable_.propValueStr(valueId);
            valueItem.doc_count = count;

            propItem.doc_count += count;
        }
    }
}

NS_FACETED_END
