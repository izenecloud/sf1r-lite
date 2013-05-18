#include "DiversityRoundEvaluator.h"
#include "../group-manager/PropValueTable.h"

using namespace sf1r;

DiversityRoundEvaluator::DiversityRoundEvaluator(
    const faceted::PropValueTable& diversityValueTable)
    : ProductScoreEvaluator("round")
    , diversityValueTable_(diversityValueTable)
    , lock_(diversityValueTable.getMutex())
    , lastCategoryScore_(0)
{
}

score_t DiversityRoundEvaluator::evaluate(ProductScore& productScore)
{
    const std::vector<score_t>& rankScores = productScore.rankScores_;

    // no "category score"
    if (rankScores.empty())
        return 0;

    // ignore multiple offer items
    if (rankScores.size() > 1 && rankScores[1] != 1)
        return 0;

    faceted::PropValueTable::PropIdList propIdList;
    diversityValueTable_.getPropIdList(productScore.docId_, propIdList);

    // ignore multiple merchants
    if (propIdList.size() != 1)
        return 0;

    // reset the round status when "category score" is changed
    const score_t categoryScore = rankScores[0];
    if (lastCategoryScore_ != categoryScore)
    {
        roundMap_.clear();
        lastCategoryScore_ = categoryScore;
    }

    const merchant_id_t merchantId = propIdList[0];
    productScore.singleMerchantId_ = merchantId;

    return --roundMap_[merchantId];
}
