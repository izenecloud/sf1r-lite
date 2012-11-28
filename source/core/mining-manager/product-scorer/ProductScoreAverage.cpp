#include "ProductScoreAverage.h"

using namespace sf1r;

ProductScoreAverage::ProductScoreAverage(const ProductScoreConfig& config)
    : ProductScoreSum(config)
{
}

score_t ProductScoreAverage::score(docid_t docId)
{
    score_t average = 0;
    std::size_t num = scorerNum_();

    if (num != 0)
    {
        score_t sum = ProductScoreSum::score(docId);
        average = sum / num;
    }

    return average;
}
