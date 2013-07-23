#include "../title-scorer/TitleScoreList.h"
#include "TitleRelevanceScorer.h"
#include <math.h>
#include <iostream>

namespace sf1r
{

TitleRelevanceScorer::TitleRelevanceScorer(
        const ProductScoreConfig& config,
        TitleScoreList* titleScoreList,
        const double& thisScore)
    : ProductScorer(config)
    , titleScoreList_(titleScoreList)
    , thisScore_(thisScore)
{

}

score_t TitleRelevanceScorer::score(docid_t docId)
{
    double thisDocScore = titleScoreList_->getScore(docId);
    double score_distance = std::abs(thisDocScore - thisScore_);

    double difPoint = 0;
    if (score_distance == 0)
        difPoint = 1;
    else
    {
        double maxscore = thisScore_*4;
        if (score_distance < maxscore)
        {
            difPoint = 1- score_distance/maxscore;
        }
        else
            difPoint = 0;
    }
    return difPoint;
}
}
