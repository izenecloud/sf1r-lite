///
/// @file NumericGroupCounter.h
/// @brief count docs for numeric property values
/// @author August Njam Grong <ran.long@izenesoft.com>
/// @date Created 2011-09-05
///

#ifndef SF1R_NUMERIC_GROUP_COUNTER_H
#define SF1R_NUMERIC_GROUP_COUNTER_H

#include "GroupCounter.h"

namespace sf1r {
class NumericPropertyTable;
}

NS_FACETED_BEGIN

class NumericGroupCounter : public GroupCounter
{
public:
    NumericGroupCounter(const NumericPropertyTable *propertyTable);

    ~NumericGroupCounter();

    virtual void addDoc(docid_t doc);

    virtual void getGroupRep(GroupRep &groupRep);

    static void toOntologyRepItemList(GroupRep &groupRep);

private:
    const NumericPropertyTable *propertyTable_;
    std::map<double, unsigned int> countTable_;
};

NS_FACETED_END

#endif
