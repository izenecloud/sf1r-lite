#include "ProductScoreSum.h"

using namespace sf1r;

ProductScoreSum::ProductScoreSum(const ProductScoreConfig& config)
    : ProductScorer(config)
{
}

ProductScoreSum::~ProductScoreSum()
{
    for (Scorers::iterator it = scorers_.begin();
         it != scorers_.end(); ++it)
    {
        delete *it;
    }
}

void ProductScoreSum::addScorer(ProductScorer* scorer)
{
    scorers_.push_back(scorer);
}

score_t ProductScoreSum::score(docid_t docId)
{
    score_t sum = 0;

    for (Scorers::iterator it = scorers_.begin();
         it != scorers_.end(); ++it)
    {
        ProductScorer* scorer = *it;
        sum += scorer->score(docId) * scorer->weight();
    }

    return sum;
}

std::size_t ProductScoreSum::scorerNum_() const
{
    return scorers_.size();
}
