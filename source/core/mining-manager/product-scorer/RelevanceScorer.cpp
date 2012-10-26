#include "RelevanceScorer.h"
#include <search-manager/DocumentIterator.h>
#include <ranking-manager/RankQueryProperty.h>
#include <ranking-manager/PropertyRanker.h>
#include <cassert>

using namespace sf1r;

namespace
{
/**
 * as the range of relevance score range is roughly in [0, 100),
 * in order to give the lowest priority to relevance score, that is,
 * make the score multiplied by weight in the range of [0, 1),
 * we choose 0.01 as its weight accordingly.
 */
const score_t kRelevanceScoreWeight = 0.01;
}

RelevanceScorer::RelevanceScorer(
    DocumentIterator* scoreDocIterator,
    const std::vector<RankQueryProperty>& rankQueryProps,
    const std::vector<boost::shared_ptr<PropertyRanker> >& propRankers)
    : ProductScorer(kRelevanceScoreWeight)
    , scoreDocIterator_(scoreDocIterator)
    , rankQueryProps_(rankQueryProps)
    , propRankers_(propRankers)
{
}

score_t RelevanceScorer::score(docid_t docId)
{
    assert(scoreDocIterator_->doc() == docId);

    return scoreDocIterator_->score(rankQueryProps_, propRankers_);
}
