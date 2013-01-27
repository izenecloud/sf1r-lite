/**
 * @file DiversityRoundEvaluator.h
 * @brief get diversity round as score.
 *
 * For example, given a list of search results, assuming each are from
 * a single merchant, and the sequence of their merchants are
 * "A A A B B B C C C".
 *
 * Then their round scores could be assigned as "-1 -2 -3 -1 -2 -3 -1 -2 -3",
 * after sorting the search results by this round score in descending order,
 * the search results would be "A B C A B C A B C".
 *
 * @author Jun Jiang
 * @date Created 2012-12-03
 */

#ifndef SF1R_DIVERSITY_ROUND_EVALUATOR_H
#define SF1R_DIVERSITY_ROUND_EVALUATOR_H

#include "ProductScoreEvaluator.h"
#include <map>

namespace sf1r
{

class DiversityRoundEvaluator : public ProductScoreEvaluator
{
public:
    DiversityRoundEvaluator();

    virtual score_t evaluate(ProductScore& productScore);

private:
    score_t lastCategoryScore_;

    std::map<merchant_id_t, int> roundMap_;
};

} // namespace sf1r

#endif // SF1R_DIVERSITY_ROUND_EVALUATOR_H
