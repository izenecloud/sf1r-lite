#include "RandomScorer.h"

using namespace sf1r;

RandomScorer::RandomScorer(
    const ProductScoreConfig& config,
    Engine& engine)
    : ProductScorer(config)
    , generator_(engine, Distribution())
{
}

score_t RandomScorer::score(docid_t docId)
{
    return generator_();
}
