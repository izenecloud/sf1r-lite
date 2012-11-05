/**
 * @file RelevanceScorer.h
 * @brief It gives relevance score.
 *
 * For example, given the relevance score 12.34, if the weight is 0.01,
 * then the relevance score multiplied by weight would be 0.1234.
 *
 * @author Jun Jiang
 * @date Created 2012-10-26
 */

#ifndef SF1R_RELEVANCE_SCORER_H
#define SF1R_RELEVANCE_SCORER_H

#include "ProductScorer.h"
#include <vector>
#include <boost/shared_ptr.hpp>

namespace sf1r
{
class DocumentIterator;
class RankQueryProperty;
class PropertyRanker;

class RelevanceScorer : public ProductScorer
{
public:
    RelevanceScorer(
        DocumentIterator* scoreDocIterator,
        const std::vector<RankQueryProperty>& rankQueryProps,
        const std::vector<boost::shared_ptr<PropertyRanker> >& propRankers);

    virtual score_t score(docid_t docId);

private:
    DocumentIterator* scoreDocIterator_;

    const std::vector<RankQueryProperty>& rankQueryProps_;
    const std::vector<boost::shared_ptr<PropertyRanker> >& propRankers_;
};

} // namespace sf1r

#endif // SF1R_RELEVANCE_SCORER_H
