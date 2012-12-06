#include "MerchantCountEvaluator.h"
#include "../group-manager/PropValueTable.h"
#include <algorithm> // min

using namespace sf1r;

namespace
{
// for multiple merchants, their merchant count score are all set to 2,
// so that they could be compared by other scores
const std::size_t kMaxMerchantCount = 2;
}

MerchantCountEvaluator::MerchantCountEvaluator(const faceted::PropValueTable& merchantValueTable)
    : ProductScoreEvaluator("mcount")
    , merchantValueTable_(merchantValueTable)
    , lock_(merchantValueTable.getMutex())
{
}

score_t MerchantCountEvaluator::evaluate(ProductScore& productScore)
{
    docid_t docId = productScore.docId_;
    faceted::PropValueTable::PropIdList merchantIdList;
    merchantValueTable_.getPropIdList(docId, merchantIdList);

    std::size_t merchantCount = std::min(merchantIdList.size(),
                                         kMaxMerchantCount);
    if (merchantCount == 1)
    {
        productScore.singleMerchantId_ = merchantIdList[0];
    }

    return merchantCount;
}
