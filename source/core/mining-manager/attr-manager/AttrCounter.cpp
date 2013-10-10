#include "AttrCounter.h"
#include <util/PriorityQueue.h>

#include <set>
#include <map>

namespace
{
/** attribute name id, and its score */
typedef std::pair<sf1r::faceted::AttrTable::nid_t, double> AttrScore;

class AttrScoreQueue : public izenelib::util::PriorityQueue<AttrScore>
{
    public:
        AttrScoreQueue(size_t size)
        {
            this->initialize(size);
        }

    protected:
        bool lessThan(const AttrScore& p1, const AttrScore& p2) const
        {
            return (p1.second < p2.second);
        }
};

}

NS_FACETED_BEGIN

AttrCounter::AttrCounter(
    const AttrTable& attrTable,
    int minValueCount)
    : attrTable_(attrTable)
    , minValueCount_(minValueCount)
    , nameDocCountTable_(attrTable.nameNum())
    , nameValueCountTable_(attrTable.nameNum())
    , valueIdNum_(attrTable.valueNum())
    , valueDocCountTable_(valueIdNum_)
{
}

void AttrCounter::addDoc(docid_t doc)
{
    std::set<AttrTable::nid_t> nameIdSet;
    AttrTable::ValueIdList valueIdList;
    attrTable_.getValueIdList(doc, valueIdList);

    for (std::size_t i = 0; i < valueIdList.size(); ++i)
    {
        AttrTable::vid_t vId = valueIdList[i];
        if (vId < valueIdNum_)
        {
            ++valueDocCountTable_[vId];

            AttrTable::nid_t nameId = attrTable_.valueId2NameId(vId);
            if (nameIdSet.insert(nameId).second)
            {
                ++nameDocCountTable_[nameId];
            }
        }
    }
}

void AttrCounter::addAttrDoc(AttrTable::nid_t nId, docid_t doc)
{
    bool findNameId = false;
    AttrTable::ValueIdList valueIdList;
    attrTable_.getValueIdList(doc, valueIdList);

    for (std::size_t i = 0; i < valueIdList.size(); ++i)
    {
        AttrTable::vid_t vId = valueIdList[i];

        if (vId < valueIdNum_ &&
            attrTable_.valueId2NameId(vId) == nId)
        {
            ++valueDocCountTable_[vId];
            findNameId = true;
        }
    }

    if (findNameId)
    {
        ++nameDocCountTable_[nId];
    }
}

double AttrCounter::getNameScore_(AttrTable::nid_t nameId) const
{
    return nameDocCountTable_[nameId];
}

double AttrCounter::getValueScore_(AttrTable::vid_t valueId) const
{
    return valueDocCountTable_[valueId];
}

void AttrCounter::getGroupRep(int topGroupNum, OntologyRep& groupRep)
{
    NameValueMap nameValueMap;
    getNameValueMap_(nameValueMap);

    std::vector<AttrTable::nid_t> topNameIds;
    getTopNameIds_(topGroupNum, topNameIds);

    generateGroupRep_(topNameIds, nameValueMap, groupRep);
}

void AttrCounter::getNameValueMap_(NameValueMap& nameValueMap)
{
    for (std::size_t valueId = 1; valueId < valueDocCountTable_.size(); ++valueId)
    {
        double score = getValueScore_(valueId);
        if (score > 0)
        {
            AttrTable::nid_t nameId = attrTable_.valueId2NameId(valueId);
            nameValueMap[nameId].insert(ScoreValueMap::value_type(score, valueId));
            ++nameValueCountTable_[nameId];
        }
    }
}

void AttrCounter::getTopNameIds_(
    int topNum,
    std::vector<AttrTable::nid_t>& topNameIds) const
{
    if (topNum == 0)
    {
        topNum = nameDocCountTable_.size();
    }

    AttrScoreQueue queue(topNum);
    for (size_t nameId = 0; nameId < nameDocCountTable_.size(); ++nameId)
    {
        double score = getNameScore_(nameId);
        if (score > 0 && nameValueCountTable_[nameId] >= minValueCount_)
        {
            AttrScore attrScore(nameId, score);
            queue.insert(attrScore);
        }
    }

    topNameIds.resize(queue.size());
    for (std::vector<sf1r::faceted::AttrTable::nid_t>::reverse_iterator rit = topNameIds.rbegin();
        rit != topNameIds.rend(); ++rit)
    {
        *rit = queue.pop().first;
    }
}

void AttrCounter::generateGroupRep_(
    const AttrNameIds& topNameIds,
    NameValueMap& nameValueMap,
    OntologyRep& groupRep) const
{
    std::list<sf1r::faceted::OntologyRepItem>& itemList = groupRep.item_list;

    for (std::vector<AttrTable::nid_t>::const_iterator nameIt = topNameIds.begin();
         nameIt != topNameIds.end(); ++nameIt)
    {
        const AttrTable::nid_t nameId = *nameIt;

        // attribute name as root node
        itemList.push_back(OntologyRepItem());
        OntologyRepItem& nameItem = itemList.back();
        nameItem.text = attrTable_.nameStr(nameId);
        nameItem.doc_count = nameDocCountTable_[nameId];
        nameItem.score = getNameScore_(nameId);

        // attribute values are sorted by score in descending order
        const ScoreValueMap& scoreValueMap = nameValueMap[nameId];
        for (ScoreValueMap::const_reverse_iterator mapIt = scoreValueMap.rbegin();
             mapIt != scoreValueMap.rend(); ++mapIt)
        {
            const double valueScore = mapIt->first;
            const AttrTable::vid_t valueId = mapIt->second;

            itemList.push_back(OntologyRepItem());
            OntologyRepItem& valueItem = itemList.back();

            // attribute values are appended as level 1
            valueItem.level = 1;
            valueItem.id = valueId;
            valueItem.text = attrTable_.valueStr(valueId);
            valueItem.doc_count = valueDocCountTable_[valueId];
            valueItem.score = valueScore;
        }
    }
}

NS_FACETED_END
