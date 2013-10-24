///
/// @file StringGroupCounter.h
/// @brief count docs for string property values
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-07-29
///

#ifndef SF1R_STRING_GROUP_COUNTER_H
#define SF1R_STRING_GROUP_COUNTER_H

#include "GroupCounter.h"
#include "PropValueTable.h"
#include "GroupRep.h"
#include "SubGroupCounter.h"

#include <util/ustring/UString.h>
#include <vector>

NS_FACETED_BEGIN

template<typename CounterType = unsigned int>
class StringGroupCounter : public GroupCounter
{
public:
    StringGroupCounter(const PropValueTable& pvTable);
    StringGroupCounter(const PropValueTable& pvTable, const CounterType& defaultCounter);
    StringGroupCounter(const StringGroupCounter& groupCounter);

    virtual StringGroupCounter* clone() const;
    virtual void addDoc(docid_t doc);
    virtual void getGroupRep(GroupRep& groupRep);
    virtual void getStringRep(GroupRep::StringGroupRep& strRep, int level);

private:
    /**
     * append all descendent nodes of @p pvId to @p itemList.
     * @param childMapTable mapping from value id to child ids
     * @param itemList the nodes are appended to this list
     * @param pvId the value id of the root node to append
     * @param level the level of the root node to append
     * @param valueStr the string value of the root node
     */
    void appendGroupRep(
        const PropValueTable::ChildMapTable& childMapTable,
        std::list<OntologyRepItem>& itemList,
        PropValueTable::pvid_t pvId,
        int level,
        const izenelib::util::UString& valueStr
    ) const;

    void addCountForParentId();

private:
    const PropValueTable& propValueTable_;

    /** map from value id to doc count */
    std::vector<CounterType> countTable_;
};

template<typename CounterType>
StringGroupCounter<CounterType>::StringGroupCounter(const PropValueTable& pvTable)
    : propValueTable_(pvTable)
    , countTable_(pvTable.propValueNum())
{
}

template<typename CounterType>
StringGroupCounter<CounterType>::StringGroupCounter(const PropValueTable& pvTable, const CounterType& defaultCounter)
    : propValueTable_(pvTable)
    , countTable_(pvTable.propValueNum(), defaultCounter)
{
}

template<typename CounterType>
StringGroupCounter<CounterType>::StringGroupCounter(const StringGroupCounter& groupCounter)
    : propValueTable_(groupCounter.propValueTable_)
    , countTable_(groupCounter.countTable_)
{
}

template<typename CounterType>
StringGroupCounter<CounterType>* StringGroupCounter<CounterType>::clone() const
{
    return new StringGroupCounter(*this);
}

template<typename CounterType>
void StringGroupCounter<CounterType>::addDoc(docid_t doc)
{
    PropValueTable::PropIdList propIdList;
    propValueTable_.getPropIdList(doc, propIdList);

    const std::size_t idNum = propIdList.size();

    if (idNum == 0)
        return;

    for (std::size_t i = 0; i < idNum; ++i)
    {
        ++countTable_[propIdList[i]];
    }

    // total doc count for this property
    ++countTable_[0];
}

template<>
void StringGroupCounter<SubGroupCounter>::addDoc(docid_t doc)
{
    PropValueTable::PropIdList propIdList;
    propValueTable_.getPropIdList(doc, propIdList);

    const std::size_t idNum = propIdList.size();

    if (idNum == 0)
        return;

    for (std::size_t i = 0; i < idNum; ++i)
    {
        SubGroupCounter& subCounter = countTable_[propIdList[i]];
        ++subCounter.count_;
        subCounter.groupCounter_->addDoc(doc);
    }

    // total doc count for this property
    ++countTable_[0].count_;
}

template<typename CounterType>
void StringGroupCounter<CounterType>::appendGroupRep(
    const PropValueTable::ChildMapTable& childMapTable,
    std::list<OntologyRepItem>& itemList,
    PropValueTable::pvid_t pvId,
    int level,
    const izenelib::util::UString& valueStr
) const
{
    itemList.push_back(faceted::OntologyRepItem(level, valueStr, 0, countTable_[pvId]));

    const PropValueTable::PropStrMap& propStrMap = childMapTable[pvId];
    for (PropValueTable::PropStrMap::const_iterator it = propStrMap.begin();
         it != propStrMap.end(); ++it)
    {
        PropValueTable::pvid_t childId = it->second;
        if (countTable_[childId])
        {
            appendGroupRep(childMapTable, itemList, childId, level+1, it->first);
        }
    }
}

template<>
void StringGroupCounter<SubGroupCounter>::appendGroupRep(
    const PropValueTable::ChildMapTable& childMapTable,
    std::list<OntologyRepItem>& itemList,
    PropValueTable::pvid_t pvId,
    int level,
    const izenelib::util::UString& valueStr
) const
{
    const SubGroupCounter& subCounter = countTable_[pvId];
    itemList.push_back(faceted::OntologyRepItem(level, valueStr, 0, subCounter.count_));

    if (pvId)
    {
        subCounter.groupCounter_->getStringRep(itemList, level+1);
    }

    const PropValueTable::PropStrMap& propStrMap = childMapTable[pvId];
    for (PropValueTable::PropStrMap::const_iterator it = propStrMap.begin();
         it != propStrMap.end(); ++it)
    {
        PropValueTable::pvid_t childId = it->second;
        if (countTable_[childId].count_)
        {
            appendGroupRep(childMapTable, itemList, childId, level+1, it->first);
        }
    }
}

template<typename CounterType>
void StringGroupCounter<CounterType>::getGroupRep(GroupRep& groupRep)
{
    addCountForParentId();

    GroupRep::StringGroupRep& itemList = groupRep.stringGroupRep_;
    izenelib::util::UString propName(propValueTable_.propName(), izenelib::util::UString::UTF_8);
    const PropValueTable::ChildMapTable& childMapTable = propValueTable_.childMapTable();

    // start from id 0 at level 0
    appendGroupRep(childMapTable, itemList, 0, 0, propName);
}

template<typename CounterType>
void StringGroupCounter<CounterType>::addCountForParentId()
{
    const std::size_t totalIdNum = countTable_.size();

    if (totalIdNum == 0)
        return;

    std::vector<PropValueTable::pvid_t> parentIds;
    std::vector<CounterType> newCountTable(totalIdNum);
    newCountTable[0] = countTable_[0];

    for (std::size_t i = 1; i < totalIdNum; ++i)
    {
        const CounterType count = countTable_[i];
        if (count == 0)
            continue;

        propValueTable_.getParentIds(i, parentIds);
        for (std::vector<PropValueTable::pvid_t>::const_iterator it = parentIds.begin();
             it != parentIds.end(); ++it)
        {
            newCountTable[*it] += count;
        }
    }

    countTable_.swap(newCountTable);
}

template<>
void StringGroupCounter<SubGroupCounter>::addCountForParentId()
{
    // SubGroupCounter does not support count for parent ids
}

template<typename CounterType>
void StringGroupCounter<CounterType>::getStringRep(GroupRep::StringGroupRep& strRep, int level)
{
    addCountForParentId();

    const PropValueTable::ChildMapTable& childMapTable = propValueTable_.childMapTable();
    const PropValueTable::PropStrMap& propStrMap = childMapTable[0];

    for (PropValueTable::PropStrMap::const_iterator it = propStrMap.begin();
        it != propStrMap.end(); ++it)
    {
        PropValueTable::pvid_t childId = it->second;
        if (countTable_[childId])
        {
            appendGroupRep(childMapTable, strRep, childId, level, it->first);
        }
    }
}

template<>
void StringGroupCounter<SubGroupCounter>::getStringRep(GroupRep::StringGroupRep& strRep, int level)
{
    addCountForParentId();

    const PropValueTable::ChildMapTable& childMapTable = propValueTable_.childMapTable();
    const PropValueTable::PropStrMap& propStrMap = childMapTable[0];

    for (PropValueTable::PropStrMap::const_iterator it = propStrMap.begin();
        it != propStrMap.end(); ++it)
    {
        PropValueTable::pvid_t childId = it->second;
        if (countTable_[childId].count_)
        {
            appendGroupRep(childMapTable, strRep, childId, level, it->first);
        }
    }
}

NS_FACETED_END

#endif
