#include "ZambeziScoreNormalizer.h"
#include <search-manager/NumericPropertyTableBuilder.h>
#include <mining-manager/MiningManager.h>
#include <mining-manager/attr-manager/AttrTable.h>
#include <util/fmath/fmath.hpp>

namespace sf1r
{

ZambeziScoreNormalizer::ZambeziScoreNormalizer(MiningManager& miningManager)
        : numericTableBuilder_(*miningManager.GetNumericTableBuilder())
        , attrTable_(miningManager.GetAttrTable())
{
}

void ZambeziScoreNormalizer::normalizeScore(
    const std::vector<docid_t>& docids,
    const std::vector<float>& productScores,
    std::vector<float>& relevanceScores,
    PropSharedLockSet& sharedLockSet)
{
    if (attrTable_)
    {
        sharedLockSet.insertSharedLock(attrTable_);
    }

    std::string propName = "itemcount";
    boost::shared_ptr<NumericPropertyTableBase> numericTable =
            numericTableBuilder_.createPropertyTable(propName);

    if (numericTable)
    {
        sharedLockSet.insertSharedLock(numericTable.get());
    }

    for (uint32_t i = 0; i < docids.size(); ++i)
    {
        int32_t itemcount = 1;
        if (numericTable)
        {
            numericTable->getInt32Value(docids[i], itemcount, false);
        }

        uint32_t attr_size = 1;
        if (attrTable_)
        {
            faceted::AttrTable::ValueIdList attrvids;
            attrTable_->getValueIdList(docids[i], attrvids);
            attr_size += std::min(attrvids.size(), size_t(30))*10.;
        }
        relevanceScores[i] += attr_size;
    }

    for (unsigned int i = 0; i < relevanceScores.size(); ++i)
    {
        float x = fmath::exp((float)(relevanceScores[i]/60000.*-1));
        x = (1-x)/(1+x);
        relevanceScores[i] = int((x*100. + productScores[i])/10+0.5)*10;
    }
}

}
