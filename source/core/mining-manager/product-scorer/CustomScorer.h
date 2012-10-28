/**
 * @file CustomScorer.h
 * @brief It gives score according to customized ranks.
 *
 * For example, docid [1, 2, 3] are customized as top ranks in order,
 * then their scores would be [3, 2, 1], other docid's score is zero.
 *
 * If the score weight is 10, then the scores multiplied by weight
 * would be [30, 20, 10] accordingly.
 *
 * @author Jun Jiang
 * @date Created 2012-10-26
 */

#ifndef SF1R_CUSTOM_SCORER_H
#define SF1R_CUSTOM_SCORER_H

#include "ProductScorer.h"
#include <vector>
#include <map>

namespace sf1r
{

class CustomScorer : public ProductScorer
{
public:
    CustomScorer(const std::vector<docid_t>& topIds);

    virtual score_t score(docid_t docId);

private:
    typedef std::map<docid_t, score_t> ScoreMap;
    ScoreMap scoreMap_;
};

} // namespace sf1r

#endif // SF1R_CUSTOM_SCORER_H
