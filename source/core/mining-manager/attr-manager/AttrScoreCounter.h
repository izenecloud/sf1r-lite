///
/// @file AttrScoreCounter.h
/// @brief count docs for attribute values, sort by attr name score
///

#ifndef SF1R_ATTR_SCORE_COUNTER_H
#define SF1R_ATTR_SCORE_COUNTER_H

#include "AttrCounter.h"

namespace sf1r
{
class AttrTokenizeWrapper;
}

NS_FACETED_BEGIN

class PropValueTable;

class AttrScoreCounter : public AttrCounter
{
public:
    AttrScoreCounter(
        const AttrTable& attrTable,
        const PropValueTable& categoryValueTable);

    virtual void addDoc(docid_t doc);

protected:
    virtual double getNameScore_(AttrTable::nid_t nameId) const;

    virtual double getValueScore_(AttrTable::vid_t valueId) const;

private:
    void nameStr_(AttrTable::nid_t nameId, std::string& nameStr) const;

    void valueStr_(AttrTable::vid_t valueId, std::string& valueStr) const;

    void categoryStr_(docid_t doc, std::string& categoryStr) const;

private:
    const PropValueTable& categoryValueTable_;

    AttrTokenizeWrapper& attrTokenizeWrapper_;

    /** map from name id to score */
    std::vector<double> nameScoreTable_;

    /** map from value id to score */
    std::vector<double> valueScoreTable_;
};

NS_FACETED_END

#endif
