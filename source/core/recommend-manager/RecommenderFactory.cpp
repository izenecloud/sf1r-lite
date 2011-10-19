#include "RecommenderFactory.h"

namespace sf1r
{

RecommenderFactory::RecommenderFactory(
    ItemManager& itemManager,
    VisitManager& visitManager,
    PurchaseManager& purchaseManager,
    CartManager& cartManager,
    OrderManager& orderManager,
    EventManager& eventManager,
    RateManager& rateManager,
    CoVisitManager& coVisitManager,
    ItemCFManager& itemCFManager
)
    : userEventFilter_(purchaseManager, cartManager, eventManager, rateManager)
    , fbtRecommender_(itemManager, orderManager)
    , vavRecommender_(itemManager, coVisitManager)
    , babRecommender_(itemManager, itemCFManager)
    , bobRecommender_(itemManager, itemCFManager, userEventFilter_, visitManager)
    , bosRecommender_(itemManager, itemCFManager, userEventFilter_, cartManager)
    , boeRecommender_(itemManager, itemCFManager, userEventFilter_)
    , tibRecommender_(orderManager)
{
}

Recommender* RecommenderFactory::getRecommender(RecommendType type)
{
    switch (type)
    {
        case FREQUENT_BUY_TOGETHER:
            return &fbtRecommender_;

        case BUY_ALSO_BUY:
            return &babRecommender_;

        case VIEW_ALSO_VIEW:
            return &vavRecommender_;

        case BASED_ON_EVENT:
            return &boeRecommender_;

        case BASED_ON_BROWSE_HISTORY:
            return &bobRecommender_;

        case BASED_ON_SHOP_CART:
            return &bosRecommender_;

        default:
            LOG(ERROR) << "RecommendType " << type << " is not supported yet";
    }

    return NULL;
}

TIBRecommender* RecommenderFactory::getTIBRecommender()
{
    return &tibRecommender_;
}

} // namespace sf1r
