///
/// @file AttrCounter.h
/// @brief count docs for attribute values
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-07-29
///

#ifndef SF1R_ATTR_COUNTER_H
#define SF1R_ATTR_COUNTER_H

#include "AttrTable.h"
#include "../faceted-submanager/faceted_types.h"
#include "../faceted-submanager/ontology_rep.h"
#include "../group-manager/PropSharedLockGetter.h"

#include <vector>

NS_FACETED_BEGIN

class AttrCounter : public PropSharedLockGetter
{
public:
    AttrCounter(const AttrTable& attrTable);

    virtual const PropSharedLock* getSharedLock() { return &attrTable_; }

    void addDoc(docid_t doc);

    void addAttrDoc(AttrTable::nid_t nId, docid_t doc);

    void getGroupRep(int topGroupNum, OntologyRep& groupRep) const;

private:
    const AttrTable& attrTable_;

    /** map from name id to doc count */
    std::vector<int> nameCountTable_;

    /** the number of value ids */
    const std::size_t valueIdNum_;

    /** map from value id to doc count */
    std::vector<int> valueCountTable_;
};

NS_FACETED_END

#endif 
