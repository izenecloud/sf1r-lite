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

AttrCounter::AttrCounter(const AttrTable& attrTable)
    : attrTable_(attrTable)
    , nameCountTable_(attrTable.nameNum())
    , valueIdNum_(attrTable.valueNum())
    , valueCountTable_(valueIdNum_)
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
            ++valueCountTable_[vId];

            AttrTable::nid_t nameId = attrTable_.valueId2NameId(vId);
            if (nameIdSet.insert(nameId).second)
            {
                ++nameCountTable_[nameId];
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
            ++valueCountTable_[vId];
            findNameId = true;
        }
    }

    if (findNameId)
    {
        ++nameCountTable_[nId];
    }
}

double AttrCounter::getNameScore_(AttrTable::nid_t nameId) const
{
    return nameCountTable_[nameId];
}

double AttrCounter::getValueScore_(AttrTable::vid_t valueId) const
{
    return valueCountTable_[valueId];
}

void AttrCounter::getGroupRep(int topGroupNum, OntologyRep& groupRep) const
{
    std::vector<AttrTable::nid_t> topNameIds;
    getTopNameIds_(topGroupNum, topNameIds);

    NameValueMap nameValueMap;
    getNameValueMap_(nameValueMap);

    generateGroupRep_(topNameIds, nameValueMap, groupRep);
}

void AttrCounter::getNameValueMap_(NameValueMap& nameValueMap) const
{
    for (std::size_t valueId = 1; valueId < valueCountTable_.size(); ++valueId)
    {
        double score = getValueScore_(valueId);
        if (score > 0)
        {
            AttrTable::nid_t nameId = attrTable_.valueId2NameId(valueId);
            nameValueMap[nameId][score].push_back(valueId);
        }
    }
}

void AttrCounter::getTopNameIds_(
    int topNum,
    std::vector<AttrTable::nid_t>& topNameIds) const
{
    if (topNum == 0)
    {
        topNum = nameCountTable_.size();
    }

    AttrScoreQueue queue(topNum);
    for (size_t nameId = 0; nameId < nameCountTable_.size(); ++nameId)
    {
        double score = getNameScore_(nameId);
        if (score > 0)
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
        // attribute name as root node
        itemList.push_back(OntologyRepItem());
        OntologyRepItem& nameItem = itemList.back();
        nameItem.text = attrTable_.nameStr(*nameIt);
        nameItem.doc_count = nameCountTable_[*nameIt];

        // attribute values are sort by score in descending order
        const ScoreValueMap& scoreValueMap = nameValueMap[*nameIt];
        for (ScoreValueMap::const_reverse_iterator mapIt = scoreValueMap.rbegin();
             mapIt != scoreValueMap.rend(); ++mapIt)
        {
            const AttrValueIds& valueIds = mapIt->second;

            for (AttrValueIds::const_iterator idIt = valueIds.begin();
                 idIt != valueIds.end(); ++idIt)
            {
                AttrTable::vid_t vid = *idIt;
                itemList.push_back(OntologyRepItem());
                OntologyRepItem& valueItem = itemList.back();

                // attribute values are appended as level 1
                valueItem.level = 1;

                // store id for quick find while merge attribute values.
                valueItem.id = vid;
                valueItem.text = attrTable_.valueStr(vid);
                valueItem.doc_count = valueCountTable_[vid];
            }
        }
    }
}

NS_FACETED_END
