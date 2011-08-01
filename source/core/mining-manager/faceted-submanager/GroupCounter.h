///
/// @file GroupCounter.h
/// @brief count docs for property values
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-07-29
///

#ifndef SF1R_GROUP_COUNTER_H
#define SF1R_GROUP_COUNTER_H

#include "faceted_types.h"
#include "ontology_rep.h"
#include "prop_value_table.h"

#include <vector>

NS_FACETED_BEGIN

class GroupCounter
{
public:
    GroupCounter(const PropValueTable& pvTable);

    void addDoc(docid_t doc);

    void getGroupRep(OntologyRep& groupRep);

private:
    const PropValueTable& propValueTable_;
    const std::vector<PropValueTable::pvid_t>& valueIdTable_;
    const std::vector<PropValueTable::pvid_t>& parentIdTable_;

    /** the number of value ids */
    const std::size_t valueIdNum_;

    /** map from value id to doc count */
    std::vector<int> countTable_;
};

NS_FACETED_END

#endif 
