///
/// @file NumericGroupCounter.h
/// @brief count docs for numeric property values
/// @author August Njam Grong <ran.long@izenesoft.com>
/// @date Created 2011-09-05
///

#ifndef SF1R_NUMERIC_GROUP_COUNTER_H
#define SF1R_NUMERIC_GROUP_COUNTER_H

#include <search-manager/NumericPropertyTable.h>
#include "GroupCounter.h"

NS_FACETED_BEGIN

template<typename T>
class NumericGroupCounter : public GroupCounter
{
public:
    NumericGroupCounter(const NumericPropertyTable *propertyTable) : propertyTable_(propertyTable)
    {}

    virtual void addDoc(docid_t doc)
    {
        docSet_.insert(doc);
    }

    virtual void getGroupRep(OntologyRep& groupRep) const
    {}

private:
    const NumericPropertyTable *propertyTable_;
    std::set<docid_t> docSet_;
};

NS_FACETED_END

#endif 
