/**
 * @file OfflineProductScorerFactory.h
 * @brief interface to create ProductScorer instance for offline calculation.
 * @author Jun Jiang
 * @date Created 2012-11-16
 */

#ifndef SF1R_OFFLINE_PRODUCT_SCORER_FACTORY_H
#define SF1R_OFFLINE_PRODUCT_SCORER_FACTORY_H

namespace sf1r
{
class ProductScorer;
struct ProductScoreConfig;

class OfflineProductScorerFactory
{
public:
    virtual ~OfflineProductScorerFactory() {}

    virtual ProductScorer* createScorer(
        const ProductScoreConfig& scoreConfig) = 0;
};

} // namespace sf1r

#endif // SF1R_OFFLINE_PRODUCT_SCORER_FACTORY_H
