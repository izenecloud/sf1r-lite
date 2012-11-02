/**
 * @file ScoreDocEvaluator.h
 * @brief evaluate the score values for ScoreDoc.
 * @author Jun Jiang
 * @date Created 2012-10-25
 */

#ifndef SF1R_SCORE_DOC_EVALUATOR_H
#define SF1R_SCORE_DOC_EVALUATOR_H

#include "CustomRanker.h"
#include <mining-manager/product-scorer/ProductScorer.h>
#include <boost/scoped_ptr.hpp>

namespace sf1r
{
struct ScoreDoc;

class ScoreDocEvaluator
{
public:
    ScoreDocEvaluator(
        ProductScorer* productScorer,
        CustomRankerPtr customRanker);

    void evaluate(ScoreDoc& scoreDoc);

private:
    boost::scoped_ptr<ProductScorer> productScorer_;

    CustomRankerPtr customRanker_;
};

} // namespace sf1r

#endif // SF1R_SCORE_DOC_EVALUATOR_H
