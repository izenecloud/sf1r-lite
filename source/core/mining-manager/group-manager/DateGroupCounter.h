///
/// @file DateGroupCounter.h
/// @brief count docs for date property values
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2012-07-03
///

#ifndef SF1R_DATE_GROUP_COUNTER_H
#define SF1R_DATE_GROUP_COUNTER_H

#include "GroupCounter.h"
#include "DateGroupTable.h"
#include "DateStrParser.h"
#include "GroupRep.h"
#include "SubGroupCounter.h"

#include <util/ustring/UString.h>
#include <map>
#include <utility>

NS_FACETED_BEGIN

template<typename CounterType = unsigned int>
class DateGroupCounter : public GroupCounter
{
public:
    DateGroupCounter(const DateGroupTable& dateTable, DATE_MASK_TYPE mask);
    DateGroupCounter(const DateGroupTable& dateTable, DATE_MASK_TYPE mask, const CounterType& subCounter);
    DateGroupCounter(const DateGroupCounter& groupCounter);

    virtual DateGroupCounter* clone() const;
    virtual void addDoc(docid_t doc);
    virtual void getGroupRep(GroupRep& groupRep);
    virtual void getStringRep(GroupRep::StringGroupRep& strRep, int level) const;
    virtual void insertSharedLock(SharedLockSet& lockSet) const;

private:
    const DateGroupTable& dateTable_;
    const DATE_MASK_TYPE mask_;

    /** map from date to doc count */
    typedef std::map<DateGroupTable::date_t, CounterType> CountTable;
    CountTable countTable_;

    unsigned int totalCount_;

    std::pair<DateGroupTable::date_t, const CounterType> initSubCounterPair_;

    DateGroupTable::DateSet dateSet_;
};

template<typename CounterType>
DateGroupCounter<CounterType>::DateGroupCounter(const DateGroupTable& dateTable, DATE_MASK_TYPE mask)
    : dateTable_(dateTable)
    , mask_(mask)
    , totalCount_(0)
    , initSubCounterPair_(0, 0)
{
}

template<typename CounterType>
DateGroupCounter<CounterType>::DateGroupCounter(const DateGroupTable& dateTable, DATE_MASK_TYPE mask, const CounterType& subCounter)
    : dateTable_(dateTable)
    , mask_(mask)
    , totalCount_(0)
    , initSubCounterPair_(0, subCounter)
{
}

template<typename CounterType>
DateGroupCounter<CounterType>::DateGroupCounter(const DateGroupCounter& groupCounter)
    : dateTable_(groupCounter.dateTable_)
    , mask_(groupCounter.mask_)
    , countTable_(groupCounter.countTable_)
    , totalCount_(groupCounter.totalCount_)
    , initSubCounterPair_(groupCounter.initSubCounterPair_)
{
}

template<typename CounterType>
DateGroupCounter<CounterType>* DateGroupCounter<CounterType>::clone() const
{
    return new DateGroupCounter(*this);
}

template<typename CounterType>
void DateGroupCounter<CounterType>::addDoc(docid_t doc)
{
    dateSet_.clear();
    dateTable_.getDateSet(doc, mask_, dateSet_);

    for (DateGroupTable::DateSet::const_iterator it = dateSet_.begin();
        it != dateSet_.end(); ++it)
    {
        ++countTable_[*it];
    }

    // total doc count for this property
    if (!dateSet_.empty())
    {
        ++totalCount_;
    }
}

template<>
void DateGroupCounter<SubGroupCounter>::addDoc(docid_t doc)
{
    dateSet_.clear();
    dateTable_.getDateSet(doc, mask_, dateSet_);

    for (DateGroupTable::DateSet::const_iterator it = dateSet_.begin();
        it != dateSet_.end(); ++it)
    {
        SubGroupCounter& subCounter = countTable_.insert(initSubCounterPair_).first->second;
        ++subCounter.count_;
        subCounter.groupCounter_->addDoc(doc);
    }

    // total doc count for this property
    if (!dateSet_.empty())
    {
        ++totalCount_;
    }
}

template<typename CounterType>
void DateGroupCounter<CounterType>::getGroupRep(GroupRep& groupRep)
{
    GroupRep::StringGroupRep& itemList = groupRep.stringGroupRep_;
    izenelib::util::UString propName(dateTable_.propName(), izenelib::util::UString::UTF_8);
    itemList.push_back(faceted::OntologyRepItem(0, propName, 0, totalCount_));

    // each date at level 1
    getStringRep(itemList, 1);
}

template<typename CounterType>
void DateGroupCounter<CounterType>::getStringRep(GroupRep::StringGroupRep& strRep, int level) const
{
    std::string dateStr;
    izenelib::util::UString dateUStr;
    DateStrParser* dateStrParser = DateStrParser::get();

    for (typename CountTable::const_iterator it = countTable_.begin();
        it != countTable_.end(); ++it)
    {
        dateStrParser->dateToAPIStr(it->first, dateStr);
        dateUStr.assign(dateStr, izenelib::util::UString::UTF_8);
        strRep.push_back(faceted::OntologyRepItem(level, dateUStr, 0, it->second));
    }
}

template<>
void DateGroupCounter<SubGroupCounter>::getStringRep(GroupRep::StringGroupRep& strRep, int level) const
{
    std::string dateStr;
    izenelib::util::UString dateUStr;
    DateStrParser* dateStrParser = DateStrParser::get();

    for (CountTable::const_iterator it = countTable_.begin();
        it != countTable_.end(); ++it)
    {
        dateStrParser->dateToAPIStr(it->first, dateStr);
        dateUStr.assign(dateStr, izenelib::util::UString::UTF_8);
        const SubGroupCounter& subCounter = it->second;

        strRep.push_back(faceted::OntologyRepItem(level, dateUStr, 0, subCounter.count_));
        subCounter.groupCounter_->getStringRep(strRep, level+1);
    }
}

template<typename CounterType>
void DateGroupCounter<CounterType>::insertSharedLock(SharedLockSet& lockSet) const
{
    lockSet.insert(&dateTable_);
}

template<>
void DateGroupCounter<SubGroupCounter>::insertSharedLock(SharedLockSet& lockSet) const
{
    lockSet.insert(&dateTable_);

    // insert lock for SubGroupCounter
    initSubCounterPair_.second.groupCounter_->insertSharedLock(lockSet);
}

NS_FACETED_END

#endif // SF1R_DATE_GROUP_COUNTER_H
