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
    virtual void getStringRep(GroupRep::StringGroupRep& strRep, int level) const;
    virtual void insertSharedLock(SharedLockSet& lockSet) const;

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

private:
    const PropValueTable& propValueTable_;

    /** map from value id to doc count */
    std::vector<CounterType> countTable_;
    NS_BOOST_MEMORY::block_pool recycle_;
    boost::scoped_alloc alloc_;
    mutable PropValueTable::ParentSetType parentSet_;
};

template<typename CounterType>
StringGroupCounter<CounterType>::StringGroupCounter(const PropValueTable& pvTable)
    : propValueTable_(pvTable)
    , countTable_(pvTable.propValueNum())
    , alloc_(recycle_)
    , parentSet_(std::less<PropValueTable::pvid_t>(), alloc_)
{
}

template<typename CounterType>
StringGroupCounter<CounterType>::StringGroupCounter(const PropValueTable& pvTable, const CounterType& defaultCounter)
    : propValueTable_(pvTable)
    , countTable_(pvTable.propValueNum(), defaultCounter)
    , alloc_(recycle_)
    , parentSet_(std::less<PropValueTable::pvid_t>(), alloc_)
{
}

template<typename CounterType>
StringGroupCounter<CounterType>::StringGroupCounter(const StringGroupCounter& groupCounter)
    : propValueTable_(groupCounter.propValueTable_)
    , countTable_(groupCounter.countTable_)
    , alloc_(recycle_)
    , parentSet_(std::less<PropValueTable::pvid_t>(), alloc_)
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

template<>
void StringGroupCounter<SubGroupCounter>::addDoc(docid_t doc)
{
    parentSet_.clear();
    propValueTable_.parentIdSet(doc, parentSet_);

    for (PropValueTable::ParentSetType::const_iterator it = parentSet_.begin();
        it != parentSet_.end(); ++it)
    {
        SubGroupCounter& subCounter = countTable_[*it];
        ++subCounter.count_;
        subCounter.groupCounter_->addDoc(doc);
    }

    // total doc count for this property
    if (!parentSet_.empty())
    {
        ++countTable_[0].count_;
    }
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
    GroupRep::StringGroupRep& itemList = groupRep.stringGroupRep_;
    izenelib::util::UString propName(propValueTable_.propName(), izenelib::util::UString::UTF_8);
    const PropValueTable::ChildMapTable& childMapTable = propValueTable_.childMapTable();

    // start from id 0 at level 0
    appendGroupRep(childMapTable, itemList, 0, 0, propName);
}

template<typename CounterType>
void StringGroupCounter<CounterType>::getStringRep(GroupRep::StringGroupRep& strRep, int level) const
{
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
void StringGroupCounter<SubGroupCounter>::getStringRep(GroupRep::StringGroupRep& strRep, int level) const
{
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

template<typename CounterType>
void StringGroupCounter<CounterType>::insertSharedLock(SharedLockSet& lockSet) const
{
    lockSet.insert(&propValueTable_);
}

template<>
void StringGroupCounter<SubGroupCounter>::insertSharedLock(SharedLockSet& lockSet) const
{
    lockSet.insert(&propValueTable_);

    // insert lock for SubGroupCounter
    countTable_[0].groupCounter_->insertSharedLock(lockSet);
}

NS_FACETED_END

#endif
