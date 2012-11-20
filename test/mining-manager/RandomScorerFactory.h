/**
 * @file RandomScorerFactory.h
 * @brief create RandomScorer instance.
 * @author Jun Jiang
 * @date Created 2012-11-20
 */

#ifndef SF1R_RANDOM_SCORER_FACTORY_H
#define SF1R_RANDOM_SCORER_FACTORY_H

#include <mining-manager/product-score-manager/OfflineProductScorerFactory.h>
#include "RandomScorer.h"

namespace sf1r
{

class RandomScorerFactory : public OfflineProductScorerFactory
{
public:
    RandomScorerFactory(unsigned int seed = 0);

    virtual ProductScorer* createScorer(
        const ProductScoreConfig& scoreConfig);

private:
    RandomScorer::Engine engine_;
};

} // namespace sf1r

#endif // SF1R_RANDOM_SCORER_FACTORY_H
