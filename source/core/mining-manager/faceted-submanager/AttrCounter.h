///
/// @file AttrCounter.h
/// @brief count docs for attribute values
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-07-29
///

#ifndef SF1R_ATTR_COUNTER_H
#define SF1R_ATTR_COUNTER_H

#include "faceted_types.h"
#include "attr_table.h"
#include "ontology_rep.h"

#include <vector>

NS_FACETED_BEGIN

class AttrCounter
{
public:
    AttrCounter(const AttrTable& attrTable);

    void addDoc(docid_t doc);

    void addAttrDoc(AttrTable::nid_t nId, docid_t doc);

    void getGroupRep(int topGroupNum, OntologyRep& groupRep) const;

private:
    const AttrTable& attrTable_;
    const AttrTable::ValueIdTable& valueIdTable_;

    /** map from name id to doc count */
    std::vector<int> nameCountTable_;

    /** the number of value ids */
    const std::size_t valueIdNum_;

    /** map from value id to doc count */
    std::vector<int> valueCountTable_;
};

NS_FACETED_END

#endif 
