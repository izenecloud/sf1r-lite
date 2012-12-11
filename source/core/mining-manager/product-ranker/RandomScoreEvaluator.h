/**
 * @file RandomScoreEvaluator.h
 * @brief get random score in range [0, 1).
 * @author Jun Jiang
 * @date Created 2012-12-11
 */

#ifndef SF1R_RANDOM_SCORE_EVALUATOR_H
#define SF1R_RANDOM_SCORE_EVALUATOR_H

#include "ProductScoreEvaluator.h"
#include <boost/random.hpp>

namespace sf1r
{

class RandomScoreEvaluator : public ProductScoreEvaluator
{
public:
    RandomScoreEvaluator();

    virtual score_t evaluate(ProductScore& productScore);

private:
    typedef boost::mt19937 EngineT;
    typedef boost::uniform_01<score_t> DistributionT;
    typedef boost::variate_generator<EngineT&, DistributionT> GeneratorT;

    EngineT engine_;
    GeneratorT generator_;
};

} // namespace sf1r

#endif // SF1R_RANDOM_SCORE_EVALUATOR_H
