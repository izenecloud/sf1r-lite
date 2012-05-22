#include "MerchantDiversityReranker.h"
#include "MerchantDiversityRange.h"

#include <algorithm> // stable_sort

namespace sf1r
{

MerchantDiversityReranker::MerchantDiversityReranker(int merchantCountIndex)
    : merchantCountIndex_(merchantCountIndex)
{
}

void MerchantDiversityReranker::rerank(ProductScoreMatrix& scoreMatrix)
{
    ProductScoreRange range =
        findDiversityRange_(scoreMatrix.begin(), scoreMatrix.end());

    while (range.first != range.second)
    {
        std::stable_sort(range.first, range.second, compareProductScoreListByDiversityRound);

        range = findDiversityRange_(range.second, scoreMatrix.end());
    }
}

ProductScoreRange MerchantDiversityReranker::findDiversityRange_(
    ProductScoreIter first,
    ProductScoreIter last
) const
{
    MerchantDiversityRange diversityRange(merchantCountIndex_);

    for (ProductScoreIter it = first; it != last; ++it)
    {
        if (! diversityRange.addScoreIter(it))
            break;
    }

    return diversityRange.getScoreRange();
}

} // namespace sf1r
