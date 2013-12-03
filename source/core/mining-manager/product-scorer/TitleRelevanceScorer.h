/**
 * @file TitleRelevanceScorer.h
 * @brief It gives score according to query's product tokenize score.
 *
 */

#ifndef SF1R_TITLE_RELEVANCE_SCORER_H
#define SF1R_TITLE_RELEVANCE_SCORER_H

#include "ProductScorer.h"

#include <map>

namespace sf1r
{
class TitleScoreList;

class TitleRelevanceScorer : public ProductScorer
{
public:
    TitleRelevanceScorer(
        const ProductScoreConfig& config,
        TitleScoreList* titleScoreList,
        const double& thisScore);

    virtual score_t score(docid_t docId);

private:
    TitleScoreList* titleScoreList_;

    double thisScore_;
};

} // namespace sf1r

#endif // SF1R_TITLE_RELEVANCE_SCORER_H
