#include "ScoreDocEvaluator.h"
#include "ScoreDoc.h"

using namespace sf1r;

namespace
{
const double kDefaultScore = 1.0;
}

ScoreDocEvaluator::ScoreDocEvaluator(
    ProductScorer* productScorer,
    CustomRankerPtr customRanker)
    : productScorer_(productScorer)
    , customRanker_(customRanker)
{
}

void ScoreDocEvaluator::evaluate(ScoreDoc& scoreDoc)
{
    if (productScorer_)
    {
        scoreDoc.score = productScorer_->score(scoreDoc.docId);
    }
    else
    {
        scoreDoc.score = kDefaultScore;
    }

    if (customRanker_)
    {
        scoreDoc.custom_score = customRanker_->evaluate(scoreDoc.docId);
    }
}
