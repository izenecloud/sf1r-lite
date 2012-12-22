/**
 * @file ProductScoreParam.h
 * @brief the parameters used to create ProductScorer instance.
 * @author Jun Jiang
 * @date Created 2012-12-22
 */

#ifndef SF1R_PRODUCT_SCORE_PARAM_H
#define SF1R_PRODUCT_SCORE_PARAM_H

#include <string>

namespace sf1r
{
class ProductScorer;
namespace faceted { class PropSharedLockSet; }

struct ProductScoreParam
{
    /** used to create @c CustomScorer and @c CategoryScorer */
    const std::string& query_;

    /** used to create @c CategoryScorer */
    const std::string& querySource_;

    /** for concurrent access on category data by @c CategoryScorer */
    faceted::PropSharedLockSet& propSharedLockSet_;

    /** the relevance scorer, it could be NULL if not existed */
    ProductScorer* relevanceScorer_;

    ProductScoreParam(
        const std::string& query,
        const std::string& querySource,
        faceted::PropSharedLockSet& propSharedLockSet,
        ProductScorer* relevanceScorer)
        : query_(query)
        , querySource_(querySource)
        , propSharedLockSet_(propSharedLockSet)
        , relevanceScorer_(relevanceScorer)
    {}
};

} // namespace sf1r

#endif // SF1R_PRODUCT_SCORE_PARAM_H
