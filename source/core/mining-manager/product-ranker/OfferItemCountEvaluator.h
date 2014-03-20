/**
 * @file OfferItemCountEvaluator.h
 * @brief get offer item count as score.
 *
 * That is:
 * score 0 for no offer item;
 * score 1 for single offer item;
 * score 2 for multiple offer items;
 */

#ifndef SF1R_OFFER_ITEM_COUNT_EVALUATOR_H
#define SF1R_OFFER_ITEM_COUNT_EVALUATOR_H

#include "ProductScoreEvaluator.h"
#include <common/PropSharedLock.h>
#include <boost/shared_ptr.hpp>

namespace sf1r
{
class NumericPropertyTableBase;

class OfferItemCountEvaluator : public ProductScoreEvaluator
{
public:
    typedef boost::shared_ptr<const NumericPropertyTableBase> OfferCountTablePtr;

    OfferItemCountEvaluator(const OfferCountTablePtr& offerCountTable);

    virtual score_t evaluate(ProductScore& productScore);

private:
    OfferCountTablePtr offerCountTable_;

    PropSharedLock::ScopedReadLock lock_;
};

} // namespace sf1r

#endif // SF1R_OFFER_ITEM_COUNT_EVALUATOR_H
