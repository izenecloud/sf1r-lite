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

#include <vector>

NS_FACETED_BEGIN

class AttrCounter
{
public:
    AttrCounter(const AttrTable& attrTable);

    virtual ~AttrCounter() {}

    virtual void addDoc(docid_t doc);

    virtual void addAttrDoc(AttrTable::nid_t nId, docid_t doc);

    void getGroupRep(int topGroupNum, OntologyRep& groupRep) const;

protected:
    virtual double getNameScore_(AttrTable::nid_t nameId) const;

    virtual double getValueScore_(AttrTable::vid_t valueId) const;

    typedef std::vector<AttrTable::nid_t> AttrNameIds;

    void getTopNameIds_(
        int topNum,
        AttrNameIds& topNameIds) const;

    typedef std::vector<AttrTable::vid_t> AttrValueIds;
    typedef std::map<double, AttrValueIds> ScoreValueMap;
    typedef std::map<AttrTable::nid_t, ScoreValueMap> NameValueMap;

    void getNameValueMap_(NameValueMap& nameValueMap) const;

    void generateGroupRep_(
        const AttrNameIds& topNameIds,
        NameValueMap& nameValueMap,
        OntologyRep& groupRep) const;

protected:
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
