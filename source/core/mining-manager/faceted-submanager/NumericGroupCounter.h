///
/// @file NumericGroupCounter.h
/// @brief count docs for numeric property values
/// @author August Njam Grong <ran.long@izenesoft.com>
/// @date Created 2011-09-05
///

#ifndef SF1R_NUMERIC_GROUP_COUNTER_H
#define SF1R_NUMERIC_GROUP_COUNTER_H

#include <search-manager/NumericPropertyTable.h>
#include <util/ustring/UString.h>
#include "GroupCounter.h"

NS_FACETED_BEGIN

template<typename T>
class NumericGroupCounter : public GroupCounter
{
public:
    NumericGroupCounter(const NumericPropertyTable *propertyTable)
        : propertyTable_(propertyTable)
    {}

    ~NumericGroupCounter()
    {
        delete propertyTable_;
    }

    virtual void addDoc(docid_t doc)
    {
        T value;
        propertyTable_->getPropertyValue(doc, value);
        ++count_;
        ++countTable_[value];
    }

    virtual void getGroupRep(OntologyRep& groupRep) const
    {
        std::list<OntologyRepItem>& itemList = groupRep.item_list;

        izenelib::util::UString propName(propertyTable_->getPropertyName(), UString::UTF_8);
        itemList.push_back(faceted::OntologyRepItem(0, propName, 0, count_));

        for (typename std::map<T, size_t>::const_iterator it = countTable_.begin();
            it != countTable_.end(); it++)
        {
            itemList.push_back(faceted::OntologyRepItem());
            faceted::OntologyRepItem& repItem = itemList.back();
            repItem.level = 1;
            std::stringstream ss;
            ss << it->first;
            izenelib::util::UString stringValue(ss.str(), UString::UTF_8);
            repItem.text = stringValue;
            repItem.doc_count = it->second;
        }
    }

private:
    const NumericPropertyTable *propertyTable_;
    size_t count_;
    std::map<T, size_t> countTable_;
};

NS_FACETED_END

#endif 
