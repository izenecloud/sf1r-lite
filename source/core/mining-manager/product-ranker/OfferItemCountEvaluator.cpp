#include "OfferItemCountEvaluator.h"
#include <common/NumericPropertyTableBase.h>
#include <algorithm> // min

using namespace sf1r;

namespace
{
// for multiple offer items, their count scores are all set to 2,
// so that they could be compared by other scores
const int32_t kMaxOfferItemCount = 2;
}

OfferItemCountEvaluator::OfferItemCountEvaluator(const OfferCountTablePtr& offerCountTable)
    : ProductScoreEvaluator("ocount")
    , offerCountTable_(offerCountTable)
    , lock_(offerCountTable->getMutex())
{
}

score_t OfferItemCountEvaluator::evaluate(ProductScore& productScore)
{
    docid_t docId = productScore.docId_;
    int32_t offerCount = 0;

    if (!offerCountTable_->getInt32Value(docId, offerCount, false))
        return 0;

    return std::min(offerCount, kMaxOfferItemCount);
}
