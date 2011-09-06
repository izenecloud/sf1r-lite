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

    virtual void addDoc(docid_t doc)
    {
        T value;
        propertyTable_->getPropertyValue(doc, value);
        if (countTable_.find(value) == countTable_.end())
        {
            countTable_[value] = 1;
        }
        else
        {
            countTable_[value]++;
        }
    }

    virtual void getGroupRep(OntologyRep& groupRep) const
    {
        std::list<OntologyRepItem>& itemList = groupRep.item_list;

        for (typename std::map<T, size_t>::const_iterator it = countTable_.begin();
            it != countTable_.end(); it++)
        {
            itemList.push_back(faceted::OntologyRepItem());
            faceted::OntologyRepItem& repItem = itemList.back();
            repItem.level = 0;
            std::stringstream ss;
            ss << it->first;
            izenelib::util::UString propName(ss.str(), UString::UTF_8);
            repItem.text = propName;
            repItem.doc_count = it->second;
        }
    }

private:
    const NumericPropertyTable *propertyTable_;
    std::map<T, size_t> countTable_;
};

NS_FACETED_END

#endif 
