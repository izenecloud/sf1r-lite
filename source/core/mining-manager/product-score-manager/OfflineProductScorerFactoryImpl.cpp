#include "OfflineProductScorerFactoryImpl.h"
#include "../product-scorer/ProductScoreAverage.h"
#include "../product-scorer/NumericPropertyScorer.h"
#include "../faceted-submanager/ctr_manager.h"
#include "../MiningManager.h"
#include <search-manager/SearchManager.h>
#include <configuration-manager/ProductScoreConfig.h>
#include <memory> // auto_ptr
#include <glog/logging.h>

using namespace sf1r;

OfflineProductScorerFactoryImpl::OfflineProductScorerFactoryImpl(
    MiningManager& miningManager)
    : searchManager_(miningManager.GetSearchManager())
    , ctrManager_(miningManager.GetCtrManager())
{
}

ProductScorer* OfflineProductScorerFactoryImpl::createScorer(
    const ProductScoreConfig& scoreConfig)
{
    switch(scoreConfig.type)
    {
    case POPULARITY_SCORE:
        return createPopularityScorer_(scoreConfig);

    default:
        return NULL;
    }
}

ProductScorer* OfflineProductScorerFactoryImpl::createPopularityScorer_(
    const ProductScoreConfig& scoreConfig)
{
    std::auto_ptr<ProductScoreAverage> averageScorer(
        new ProductScoreAverage(scoreConfig));

    for (std::size_t i = 0; i < scoreConfig.factors.size(); ++i)
    {
        const ProductScoreConfig& factorConfig = scoreConfig.factors[i];
        ProductScorer* scorer = createNumericPropertyScorer_(factorConfig);

        if (scorer)
        {
            averageScorer->addScorer(scorer);
        }
    }

    return averageScorer.release();
}

ProductScorer* OfflineProductScorerFactoryImpl::createNumericPropertyScorer_(
    const ProductScoreConfig& scoreConfig)
{
    const std::string& propName = scoreConfig.propName;
    if (propName.empty() || scoreConfig.weight == 0)
        return NULL;

    boost::shared_ptr<NumericPropertyTableBase> numericTable =
        createNumericPropertyTable_(propName);

    if (!numericTable)
    {
        LOG(WARNING) << "failed to create NumericPropertyTableBase "
                     << "for property [" << propName << "]";
        return NULL;
    }

    LOG(INFO) << "createNumericPropertyScorer_(), propName: " << propName;
    return new NumericPropertyScorer(scoreConfig, numericTable);
}

boost::shared_ptr<NumericPropertyTableBase> OfflineProductScorerFactoryImpl::createNumericPropertyTable_(
    const std::string& propName)
{
    boost::shared_ptr<NumericPropertyTableBase> numericTable;

    if (propName == faceted::CTRManager::kCtrPropName)
    {
        if (ctrManager_)
        {
            ctrManager_->loadCtrData(numericTable);
        }
        else
        {
            LOG(WARNING) << "failed to get CTRManager";
        }
    }
    else
    {
        if (searchManager_)
        {
            numericTable = searchManager_->createPropertyTable(propName);
        }
        else
        {
            LOG(WARNING) << "failed to get SearchManager";
        }
    }

    return numericTable;
}
