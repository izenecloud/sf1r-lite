#include "AttrCounter.h"
#include <util/PriorityQueue.h>

#include <set>
#include <map>

namespace
{
/** attribute name id, and its doc count */
typedef std::pair<sf1r::faceted::AttrTable::nid_t, int> AttrFreq;

class AttrFreqQueue : public izenelib::util::PriorityQueue<AttrFreq>
{
    public:
        AttrFreqQueue(size_t size)
        {
            this->initialize(size);
        }

    protected:
        bool lessThan(const AttrFreq& p1, const AttrFreq& p2) const
        {
            return (p1.second < p2.second);
        }
};

/**
 * get attribute name ids with top freq.
 * @param nameCountTable map from name id to doc count
 * @param topNum limit the size of @p topNameIds, zero for not limit size
 * @param topNameIds store the top attribute name ids
 */
void getTopNameIds(
    const std::vector<int>& nameCountTable,
    int topNum,
    std::vector<sf1r::faceted::AttrTable::nid_t>& topNameIds
)
{
    if (topNum == 0)
    {
        topNum = nameCountTable.size();
    }

    AttrFreqQueue queue(topNum);
    for (size_t nameId = 0; nameId < nameCountTable.size(); ++nameId)
    {
        if (nameCountTable[nameId])
        {
            queue.insert(AttrFreq(nameId, nameCountTable[nameId]));
        }
    }

    topNameIds.resize(queue.size());
    for (std::vector<sf1r::faceted::AttrTable::nid_t>::reverse_iterator rit = topNameIds.rbegin();
        rit != topNameIds.rend(); ++rit)
    {
        *rit = queue.pop().first;
    }
}

}

NS_FACETED_BEGIN

AttrCounter::AttrCounter(const AttrTable* attrTable)
    : attrTable_(attrTable)
    , lock_(attrTable->getMutex())
    , valueIdTable_(attrTable->valueIdTable())
    , nameCountTable_(attrTable->nameNum())
    , valueIdNum_(attrTable->valueNum())
    , valueCountTable_(valueIdNum_)
{
}

void AttrCounter::addDoc(docid_t doc)
{
    // this doc has no attr index data yet
    if (doc >= valueIdTable_.size())
        return;

    const AttrTable::ValueIdList& valueIdList = valueIdTable_[doc];
    std::set<AttrTable::nid_t> nameIdSet;

    for (AttrTable::ValueIdList::const_iterator valueIt = valueIdList.begin();
        valueIt != valueIdList.end(); ++valueIt)
    {
        AttrTable::vid_t vId = *valueIt;
        if (vId < valueIdNum_)
        {
            ++valueCountTable_[vId];

            AttrTable::nid_t nameId = attrTable_->valueId2NameId(vId);
            if (nameIdSet.insert(nameId).second)
            {
                ++nameCountTable_[nameId];
            }
        }
    }
}

void AttrCounter::addAttrDoc(AttrTable::nid_t nId, docid_t doc)
{
    // this doc has no attr index data yet
    if (doc >= valueIdTable_.size())
        return;

    const AttrTable::ValueIdList& valueIdList = valueIdTable_[doc];
    bool findNameId = false;

    for (AttrTable::ValueIdList::const_iterator valueIt = valueIdList.begin();
        valueIt != valueIdList.end(); ++valueIt)
    {
        AttrTable::vid_t vId = *valueIt;
        if (vId < valueIdNum_
            && attrTable_->valueId2NameId(vId) == nId)
        {
            ++valueCountTable_[vId];
            findNameId = true;
        }
    }

    if (findNameId)
    {
        ++nameCountTable_[nId];
    }
}

void AttrCounter::getGroupRep(int topGroupNum, OntologyRep& groupRep) const
{
    typedef std::map<AttrTable::vid_t, int> ValueCountMap;
    typedef std::map<AttrTable::nid_t, ValueCountMap> NameCountMap;
    NameCountMap nameCountMap;
    for (std::size_t valueId = 1; valueId < valueCountTable_.size(); ++valueId)
    {
        int count = valueCountTable_[valueId];
        if (count)
        {
            AttrTable::nid_t nameId = attrTable_->valueId2NameId(valueId);
            nameCountMap[nameId][valueId] = count;
        }
    }

    std::vector<AttrTable::nid_t> topNameIds;
    getTopNameIds(nameCountTable_, topGroupNum, topNameIds);

    std::list<sf1r::faceted::OntologyRepItem>& itemList = groupRep.item_list;
    for (std::vector<AttrTable::nid_t>::const_iterator nameIt = topNameIds.begin();
        nameIt != topNameIds.end(); ++nameIt)
    {
        // attribute name as root node
        itemList.push_back(OntologyRepItem());
        OntologyRepItem& nameItem = itemList.back();
        nameItem.text = attrTable_->nameStr(*nameIt);
        nameItem.doc_count = nameCountTable_[*nameIt];

        // attribute values are appended as level 1
        const ValueCountMap& valueCountMap = nameCountMap[*nameIt];
        for (ValueCountMap::const_iterator mapIt = valueCountMap.begin();
            mapIt != valueCountMap.end(); ++mapIt)
        {
            itemList.push_back(OntologyRepItem());
            OntologyRepItem& valueItem = itemList.back();
            valueItem.level = 1;
            valueItem.text = attrTable_->valueStr(mapIt->first);
            valueItem.doc_count = mapIt->second;
        }
    }
}

NS_FACETED_END
