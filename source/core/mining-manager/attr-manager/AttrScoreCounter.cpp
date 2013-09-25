#include "AttrScoreCounter.h"
#include "../group-manager/PropValueTable.h"
#include "../util/convert_ustr.h"
#include <la-manager/AttrTokenizeWrapper.h>

namespace
{
const double kMinNameScore = 0.1;
}

NS_FACETED_BEGIN

AttrScoreCounter::AttrScoreCounter(
    const AttrTable& attrTable,
    const PropValueTable& categoryValueTable)
    : AttrCounter(attrTable)
    , categoryValueTable_(categoryValueTable)
    , attrTokenizeWrapper_(*AttrTokenizeWrapper::get())
    , nameScoreTable_(attrTable.nameNum())
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
        if (vId < valueIdNum_)
        {
            AttrTable::nid_t nameId = attrTable_.valueId2NameId(vId);
            if (nameIdSet.insert(nameId).second)
            {
                double score = nameCateScore_(nameId, categoryStr);
                if (score < kMinNameScore)
                    continue;

                nameScoreTable_[nameId] += score;
                ++nameCountTable_[nameId];
            }

            ++valueCountTable_[vId];
        }
    }
}

double AttrScoreCounter::getNameScore_(AttrTable::nid_t nameId) const
{
    return nameScoreTable_[nameId];
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

double AttrScoreCounter::nameCateScore_(
    AttrTable::nid_t nameId,
    const std::string& categoryStr) const
{
    const izenelib::util::UString& ustr = attrTable_.nameStr(nameId);
    std::string nameStr;
    convert_to_str(ustr, nameStr);

    return attrTokenizeWrapper_.att_weight(nameStr, categoryStr);
}

NS_FACETED_END
