#include "RandomScorerFactory.h"

using namespace sf1r;

RandomScorerFactory::RandomScorerFactory(unsigned int seed)
    : engine_(seed)
{
}

ProductScorer* RandomScorerFactory::createScorer(
    const ProductScoreConfig& scoreConfig)
{
    return new RandomScorer(scoreConfig, engine_);
}
