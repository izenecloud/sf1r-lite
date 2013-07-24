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
    //cout <<"thisScore_: " << thisDocScore << ":" << thisScore_ << endl;

    double difPoint = 0;
    if (score_distance == 0)
        difPoint = 1;
    else if (score_distance < thisScore_)
    {
        difPoint = 1 - score_distance/thisScore_;
    }
    else
    {
        double maxscore = thisScore_*4;
        if (score_distance < maxscore)
        {
            difPoint = 1 - score_distance/maxscore;
        }
        else
            difPoint = 0;

        if (difPoint > 0.25)
            difPoint = 0.25;
    }

    int int_res = static_cast<int>(difPoint*1000);
    difPoint = (double(int_res))/1000.0;
    //cout << "difPoint:" << difPoint << endl;
    return difPoint;
}
}
