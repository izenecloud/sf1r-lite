///
/// @file NumericRangeGroupCounter.h
/// @brief count docs for numeric property and dynamically split them into ranges
/// @author August Njam Grong <ran.long@izenesoft.com>
/// @date Created 2011-09-07
///

#ifndef SF1R_NUMERIC_RANGE_GROUP_COUNTER_H
#define SF1R_NUMERIC_RANGE_GROUP_COUNTER_H

#include <search-manager/NumericPropertyTable.h>
#include <util/ustring/UString.h>
#include "GroupCounter.h"

NS_FACETED_BEGIN

class DecimalSegmentTreeNode
{
public:
    DecimalSegmentTreeNode(int start, size_t span)
        : start_(start)
        , span_(span)
        , count_(0)
        , parent_(NULL)
    {
        for (size_t i = 0; i < 10; i++)
            children_[i] = NULL;
    }

    int getStart() const
    {
        return start_;
    }

    size_t getSpan() const
    {
        return span_;
    }

    size_t getCount() const
    {
        return count_;
    }

    void insertPoint(unsigned char *digits, size_t level)
    {
        count_++;
        if (level > 1)
        {
            if (!children_[*digits])
            {
                children_[*digits] = new DecimalSegmentTreeNode(start_ + (*digits) * span_ / 10, span_ /10);
            }
            children_[*digits]->insertPoint(digits + 1, level - 1);
        }
    }

private:
    int start_;
    size_t span_;
    size_t count_;
    DecimalSegmentTreeNode* children_[10];
    DecimalSegmentTreeNode* parent_;
};

template <typename T>
class NumericRangeGroupCounter : public GroupCounter
{
public:
    NumericRangeGroupCounter(const NumericPropertyTable *propertyTable)
        : propertyTable_(propertyTable)
        , decimalSegmentTree_(0, 1000000000)
    {}

    ~NumericRangeGroupCounter()
    {
        delete propertyTable_;
    }

    virtual void addDoc(docid_t doc)
    {
        T value;
        propertyTable_->getPropertyValue(doc, value);
        unsigned char *digits = new unsigned char[9]();
        for (int i = 8; i >= 0; i--)
        {
            digits = value % 10;
            value /= 10;
        }
        decimalSegmentTree_.insertPoint(digits, 9);
        delete digits;
    }

    virtual void getGroupRep(OntologyRep& groupRep) const
    {
//        std::list<OntologyRepItem>& itemList = groupRep.item_list;

//        izenelib::util::UString propName(propertyTable_->getPropertyName(), UString::UTF_8);
//        itemList.push_back(faceted::OntologyRepItem(0, propName, 0, count_));

//        for (typename std::map<T, size_t>::const_iterator it = countTable_.begin();
//            it != countTable_.end(); it++)
//        {
//            itemList.push_back(faceted::OntologyRepItem());
//            faceted::OntologyRepItem& repItem = itemList.back();
//            repItem.level = 1;
//            std::stringstream ss;
//            ss << it->first;
//            izenelib::util::UString stringValue(ss.str(), UString::UTF_8);
//            repItem.text = stringValue;
//            repItem.doc_count = it->second;
//        }
    }

private:
    const NumericPropertyTable *propertyTable_;
    DecimalSegmentTreeNode decimalSegmentTree_;
};

NS_FACETED_END

#endif
