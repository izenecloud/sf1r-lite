///
/// @file NumericGroupCounter.h
/// @brief count docs for numeric property values
/// @author August Njam Grong <ran.long@izenesoft.com>
/// @date Created 2011-09-05
///

#ifndef SF1R_NUMERIC_GROUP_COUNTER_H
#define SF1R_NUMERIC_GROUP_COUNTER_H

#include "GroupCounter.h"
#include "GroupRep.h"
#include "SubGroupCounter.h"
#include <search-manager/NumericPropertyTable.h>

#include <util/ustring/UString.h>
#include <boost/scoped_ptr.hpp>

#include <map>
#include <utility>

NS_FACETED_BEGIN

template<typename CounterType = unsigned int>
class NumericGroupCounter : public GroupCounter
{
public:
    NumericGroupCounter(const NumericPropertyTable* propertyTable);
    NumericGroupCounter(const NumericPropertyTable* propertyTable, const CounterType& defaultCounter);
    NumericGroupCounter(const NumericGroupCounter& groupCounter);

    virtual NumericGroupCounter* clone() const;
    virtual void addDoc(docid_t doc);
    virtual void getGroupRep(GroupRep& groupRep);
    virtual void getStringRep(GroupRep::StringGroupRep& strRep, int level) const;

private:
    boost::scoped_ptr<const NumericPropertyTable> propertyTable_;
    std::map<double, CounterType> countTable_;
    const CounterType defaultCounter_;
};

template<typename CounterType>
NumericGroupCounter<CounterType>::NumericGroupCounter(const NumericPropertyTable *propertyTable)
    : propertyTable_(propertyTable)
    , defaultCounter_(0)
{}

template<typename CounterType>
NumericGroupCounter<CounterType>::NumericGroupCounter(const NumericPropertyTable* propertyTable, const CounterType& defaultCounter)
    : propertyTable_(propertyTable)
    , defaultCounter_(defaultCounter)
{
}

template<typename CounterType>
NumericGroupCounter<CounterType>::NumericGroupCounter(const NumericGroupCounter& groupCounter)
    : propertyTable_(new NumericPropertyTable(*groupCounter.propertyTable_))
    , countTable_(groupCounter.countTable_)
    , defaultCounter_(groupCounter.defaultCounter_)
{
}

template<typename CounterType>
NumericGroupCounter<CounterType>* NumericGroupCounter<CounterType>::clone() const
{
    return new NumericGroupCounter(*this);
}

template<typename CounterType>
void NumericGroupCounter<CounterType>::addDoc(docid_t doc)
{
    double value = 0;
    if (propertyTable_->convertPropertyValue(doc, value))
    {
        ++countTable_[value];
    }
}

template<>
void NumericGroupCounter<SubGroupCounter>::addDoc(docid_t doc)
{
    double value = 0;
    if (propertyTable_->convertPropertyValue(doc, value))
    {
        SubGroupCounter& subCounter = countTable_.insert(std::make_pair(value, defaultCounter_)).first->second;
        ++subCounter.count_;
        subCounter.groupCounter_->addDoc(doc);
    }
}

template<typename CounterType>
void NumericGroupCounter<CounterType>::getGroupRep(GroupRep &groupRep)
{
    groupRep.numericGroupRep_.push_back(make_pair(propertyTable_->getPropertyName(), list<pair<double, unsigned int> >()));
    list<pair<double, unsigned int> > &countList = groupRep.numericGroupRep_.back().second;

    for (typename std::map<double, CounterType>::const_iterator it = countTable_.begin();
        it != countTable_.end(); ++it)
    {
        countList.push_back(*it);
    }
}

template<>
void NumericGroupCounter<SubGroupCounter>::getGroupRep(GroupRep &groupRep)
{
    GroupRep::StringGroupRep& itemList = groupRep.stringGroupRep_;

    izenelib::util::UString propName(propertyTable_->getPropertyName(), UString::UTF_8);
    itemList.push_back(faceted::OntologyRepItem(0, propName, 0, 0));
    faceted::OntologyRepItem& topItem = itemList.back();
    unsigned int totalCount = 0;
    izenelib::util::UString ustr;

    for (std::map<double, SubGroupCounter>::const_iterator it = countTable_.begin();
        it != countTable_.end(); ++it)
    {
        GroupRep::formatNumericToUStr(it->first, ustr);
        const SubGroupCounter& subCounter = it->second;

        itemList.push_back(faceted::OntologyRepItem(1, ustr, 0, subCounter.count_));
        subCounter.groupCounter_->getStringRep(itemList, 2);

        totalCount += subCounter.count_;
    }
    topItem.doc_count = totalCount;
}

template<typename CounterType>
void NumericGroupCounter<CounterType>::getStringRep(GroupRep::StringGroupRep& strRep, int level) const
{
    izenelib::util::UString ustr;

    for (typename std::map<double, CounterType>::const_iterator it = countTable_.begin();
        it != countTable_.end(); ++it)
    {
        GroupRep::formatNumericToUStr(it->first, ustr);
        strRep.push_back(faceted::OntologyRepItem(level, ustr, 0, it->second));
    }
}

template<>
void NumericGroupCounter<SubGroupCounter>::getStringRep(GroupRep::StringGroupRep& strRep, int level) const
{
    izenelib::util::UString ustr;

    for (std::map<double, SubGroupCounter>::const_iterator it = countTable_.begin();
        it != countTable_.end(); ++it)
    {
        GroupRep::formatNumericToUStr(it->first, ustr);
        const SubGroupCounter& subCounter = it->second;

        strRep.push_back(faceted::OntologyRepItem(level, ustr, 0, subCounter.count_));
        subCounter.groupCounter_->getStringRep(strRep, level+1);
    }
}

NS_FACETED_END

#endif
