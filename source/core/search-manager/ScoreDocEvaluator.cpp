#include "ScoreDocEvaluator.h"
#include "DocumentIterator.h"
#include "Sorter.h"
#include "ScoreDoc.h"
#include <ranking-manager/RankQueryProperty.h>
#include <ranking-manager/PropertyRanker.h>

using namespace sf1r;

namespace
{

const double kDefaultScore = 1.0;

bool checkNeedScore(
    DocumentIterator* scoreDocIterator,
    Sorter* sorter)
{
    return scoreDocIterator &&
        sorter && sorter->requireScorer();
}

}

ScoreDocEvaluator::ScoreDocEvaluator(
    DocumentIterator* scoreDocIterator,
    Sorter* sorter,
    const std::vector<RankQueryProperty>& rankQueryProps,
    const std::vector<boost::shared_ptr<PropertyRanker> >& propRankers,
    CustomRankerPtr customRanker,
    ProductScorer* productScorer)
    : isNeedScore_(checkNeedScore(scoreDocIterator, sorter))
    , scoreDocIterator_(scoreDocIterator)
    , rankQueryProps_(rankQueryProps)
    , propRankers_(propRankers)
    , customRanker_(customRanker)
    , productScorer_(productScorer)
{
}

void ScoreDocEvaluator::evaluate(ScoreDoc& scoreDoc)
{
    if (isNeedScore_)
    {
        if (productScorer_)
        {
            scoreDoc.score = productScorer_->score(scoreDoc.docId);
        }
        else
        {
            scoreDoc.score = scoreDocIterator_->score(rankQueryProps_,
                                                      propRankers_);
        }
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
