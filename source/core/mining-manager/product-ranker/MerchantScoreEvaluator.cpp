#include "MerchantScoreEvaluator.h"
#include "../merchant-score-manager/MerchantScoreManager.h"
#include "../group-manager/PropValueTable.h"

using namespace sf1r;

namespace
{
const char* kScoreName = "mscore";
}

MerchantScoreEvaluator::MerchantScoreEvaluator(
    const MerchantScoreManager& merchantScoreManager)
    : ProductScoreEvaluator(kScoreName)
    , merchantScoreManager_(merchantScoreManager)
    , categoryValueTable_(NULL)
{
}

MerchantScoreEvaluator::MerchantScoreEvaluator(
    const MerchantScoreManager& merchantScoreManager,
    const faceted::PropValueTable& categoryValueTable)
    : ProductScoreEvaluator(kScoreName)
    , merchantScoreManager_(merchantScoreManager)
    , categoryValueTable_(&categoryValueTable)
    , lock_(categoryValueTable_->getMutex())
{
}

score_t MerchantScoreEvaluator::evaluate(ProductScore& productScore)
{
    const std::vector<score_t>& rankScores = productScore.rankScores_;

    if (rankScores.size() < 2)
        return 0;

    score_t merchantCount = rankScores[1];

    // ignore multiple merchants
    if (merchantCount != 1)
        return 0;

    category_id_t categoryId = 0;
    if (categoryValueTable_)
    {
        docid_t docId = productScore.docId_;
        categoryId = categoryValueTable_->getRootValueId(docId);
    }

    return merchantScoreManager_.getIdScore(productScore.singleMerchantId_,
                                            categoryId);
}
