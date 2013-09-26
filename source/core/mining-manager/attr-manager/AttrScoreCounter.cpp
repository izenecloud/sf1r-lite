#include "AttrScoreCounter.h"
#include "../group-manager/PropValueTable.h"
#include "../util/convert_ustr.h"
#include <la-manager/AttrTokenizeWrapper.h>

namespace
{
const int kMinValueCount = 2;
const double kMinNameScore = 0.1;
const double kMinValueScore = 0.1;
}

NS_FACETED_BEGIN

AttrScoreCounter::AttrScoreCounter(
    const AttrTable& attrTable,
    const PropValueTable& categoryValueTable)
    : AttrCounter(attrTable, kMinValueCount)
    , categoryValueTable_(categoryValueTable)
    , attrTokenizeWrapper_(*AttrTokenizeWrapper::get())
    , nameScoreTable_(attrTable.nameNum())
    , valueScoreTable_(valueIdNum_)
{
}

void AttrScoreCounter::addDoc(docid_t doc)
{
    std::string categoryStr;
    categoryStr_(doc, categoryStr);

    std::set<AttrTable::nid_t> nameIdSet;
    AttrTable::ValueIdList valueIdList;
    attrTable_.getValueIdList(doc, valueIdList);

    for (std::size_t i = 0; i < valueIdList.size(); ++i)
    {
        AttrTable::vid_t vId = valueIdList[i];

        if (vId >= valueIdNum_)
            continue;

        AttrTable::nid_t nameId = attrTable_.valueId2NameId(vId);
        std::string nameStr;
        nameStr_(nameId, nameStr);

        double nameScore = attrTokenizeWrapper_.att_name_weight(nameStr,
                                                                categoryStr);
        if (nameScore < kMinNameScore)
            continue;

        std::string valueStr;
        valueStr_(vId, valueStr);

        double valueScore = attrTokenizeWrapper_.att_value_weight(nameStr,
                                                                  valueStr,
                                                                  categoryStr);
        if (valueScore < kMinValueScore)
            continue;

        if (nameIdSet.insert(nameId).second)
        {
            nameScoreTable_[nameId] += nameScore;
            ++nameDocCountTable_[nameId];
        }

        ++valueDocCountTable_[vId];
        valueScoreTable_[vId] += valueScore;
    }
}

double AttrScoreCounter::getNameScore_(AttrTable::nid_t nameId) const
{
    return nameScoreTable_[nameId];
}

double AttrScoreCounter::getValueScore_(AttrTable::vid_t valueId) const
{
    return valueScoreTable_[valueId];
}

void AttrScoreCounter::nameStr_(AttrTable::nid_t nameId, std::string& nameStr) const
{
    const izenelib::util::UString& ustr = attrTable_.nameStr(nameId);
    convert_to_str(ustr, nameStr);
}

void AttrScoreCounter::valueStr_(AttrTable::vid_t valueId, std::string& valueStr) const
{
    const izenelib::util::UString& ustr = attrTable_.valueStr(valueId);
    convert_to_str(ustr, valueStr);
}

void AttrScoreCounter::categoryStr_(docid_t doc, std::string& categoryStr) const
{
    PropValueTable::PropIdList cateIdList;
    categoryValueTable_.getPropIdList(doc, cateIdList);

    if (cateIdList.empty())
        return;

    PropValueTable::pvid_t cateId = cateIdList[0];
    izenelib::util::UString ustr;
    categoryValueTable_.propValueStr(cateId, ustr, false);
    convert_to_str(ustr, categoryStr);
}

NS_FACETED_END
