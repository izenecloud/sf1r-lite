/**
 * @file OfflineProductScorerFactoryImpl.h
 * @brief implementation of creating ProductScorer instance for
 *        offline calculation.
 * @author Jun Jiang
 * @date Created 2012-11-16
 */

#ifndef SF1R_OFFLINE_PRODUCT_SCORER_FACTORY_IMPL_H
#define SF1R_OFFLINE_PRODUCT_SCORER_FACTORY_IMPL_H

#include "OfflineProductScorerFactory.h"
#include <string>
#include <boost/shared_ptr.hpp>

namespace sf1r
{
class SearchManager;
class NumericPropertyTableBase;
class MiningManager;
namespace faceted { class CTRManager; }

class OfflineProductScorerFactoryImpl : public OfflineProductScorerFactory
{
public:
    OfflineProductScorerFactoryImpl(MiningManager& miningManager);

    virtual ProductScorer* createScorer(
        const ProductScoreConfig& scoreConfig);

private:
    ProductScorer* createPopularityScorer_(
        const ProductScoreConfig& scoreConfig);

    ProductScorer* createNumericPropertyScorer_(
        const ProductScoreConfig& scoreConfig);

    boost::shared_ptr<NumericPropertyTableBase> createNumericPropertyTable_(
        const std::string& propName);

private:
    boost::shared_ptr<SearchManager> searchManager_;

    boost::shared_ptr<faceted::CTRManager> ctrManager_;
};

} // namespace sf1r

#endif // SF1R_OFFLINE_PRODUCT_SCORER_FACTORY_IMPL_H
