#include "RandomScoreEvaluator.h"
#include <ctime>

using namespace sf1r;

RandomScoreEvaluator::RandomScoreEvaluator()
    : ProductScoreEvaluator("random")
    , engine_(std::time(NULL))
    , generator_(engine_, DistributionT())
{
}

score_t RandomScoreEvaluator::evaluate(ProductScore& productScore)
{
    return generator_();
}
