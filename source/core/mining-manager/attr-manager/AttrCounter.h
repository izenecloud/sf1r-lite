///
/// @file AttrCounter.h
/// @brief count docs for attribute values
/// @author Jun Jiang <jun.jiang@izenesoft.com>
/// @date Created 2011-07-29
///

#ifndef SF1R_ATTR_COUNTER_H
#define SF1R_ATTR_COUNTER_H

#include "AttrTable.h"
#include "../group-manager/faceted_types.h"
#include "../group-manager/ontology_rep.h"

#include <vector>
#include <map>

NS_FACETED_BEGIN

class AttrCounter
{
public:
    AttrCounter(
        const AttrTable& attrTable,
        int minValueCount = 1,
        int maxIterCount = 0);

    virtual ~AttrCounter() {}

    virtual void addDoc(docid_t doc);

    virtual void addAttrDoc(AttrTable::nid_t nId, docid_t doc);

    void getGroupRep(int topGroupNum, OntologyRep& groupRep);

protected:
    virtual double getNameScore_(AttrTable::nid_t nameId);

    virtual double getValueScore_(AttrTable::vid_t valueId);

    typedef std::vector<AttrTable::nid_t> AttrNameIds;

    void getTopNameIds_(
        int topNum,
        AttrNameIds& topNameIds);

    typedef std::multimap<double, AttrTable::vid_t> ScoreValueMap;
    typedef std::map<AttrTable::nid_t, ScoreValueMap> NameValueMap;

    void getNameValueMap_(NameValueMap& nameValueMap);

    void generateGroupRep_(
        const AttrNameIds& topNameIds,
        NameValueMap& nameValueMap,
        OntologyRep& groupRep);

protected:
    const AttrTable& attrTable_;

    /**
     * given an attr name, if its attr value count is less than
     * @c minValueCount_, it would be excluded in final result.
     */
    const int minValueCount_;

    /**
     * the maximum count in calling @c addDoc() and @c addAttrDoc(),
     * if zero, there would be no limit in calling those functions.
     */
    const int maxIterCount_;

    /**
     * count in calling @c addDoc() and @c addAttrDoc().
     */
    int iterCount_;

    /** map from name id to doc count */
    std::map<AttrTable::nid_t, int> nameDocCountTable_;

    /** map from name id to value count */
    typedef std::map<AttrTable::nid_t, int> NameValueCountTable;
    NameValueCountTable nameValueCountTable_;

    /** map from value id to doc count */
    typedef std::map<AttrTable::vid_t, int> ValueDocCountTable;
    ValueDocCountTable valueDocCountTable_;
};

NS_FACETED_END

#endif
