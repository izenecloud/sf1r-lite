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
    const merchant_id_t singleMerchantId = productScore.singleMerchantId_;

    // ignore multiple merchants
    if (singleMerchantId == 0)
        return 0;

    category_id_t categoryId = 0;
    if (categoryValueTable_)
    {
        docid_t docId = productScore.docId_;
        categoryId = categoryValueTable_->getRootValueId(docId);
    }

    return merchantScoreManager_.getIdScore(singleMerchantId, categoryId);
}
