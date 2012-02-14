#include "RecommenderFactory.h"
#include "FBTRecommender.h"
#include "VAVRecommender.h"
#include "BABRecommender.h"
#include "BOBRecommender.h"
#include "BOSRecommender.h"
#include "BOERecommender.h"
#include "BORRecommender.h"

#include <boost/algorithm/string/case_conv.hpp>

namespace sf1r
{

RecommenderFactory::RecommenderFactory(
    ItemManager& itemManager,
    ItemIdGenerator& itemIdGenerator,
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
    , tibRecommender_(orderManager)
    , recommenders_(RECOMMEND_TYPE_NUM)
{
    recTypeMap_["FBT"] = FREQUENT_BUY_TOGETHER;
    recTypeMap_["BAB"] = BUY_ALSO_BUY;
    recTypeMap_["VAV"] = VIEW_ALSO_VIEW;
    recTypeMap_["BOE"] = BASED_ON_EVENT;
    recTypeMap_["BOB"] = BASED_ON_BROWSE_HISTORY;
    recTypeMap_["BOS"] = BASED_ON_SHOP_CART;
    recTypeMap_["BOR"] = BASED_ON_RANDOM;

    recommenders_[FREQUENT_BUY_TOGETHER] = new FBTRecommender(itemManager, orderManager);
    recommenders_[BUY_ALSO_BUY] = new BABRecommender(itemManager, itemCFManager);
    recommenders_[VIEW_ALSO_VIEW] = new VAVRecommender(itemManager, coVisitManager);
    recommenders_[BASED_ON_EVENT] = new BOERecommender(itemManager, itemCFManager, userEventFilter_);
    recommenders_[BASED_ON_BROWSE_HISTORY] = new BOBRecommender(itemManager, itemCFManager, userEventFilter_, visitManager);
    recommenders_[BASED_ON_SHOP_CART] = new BOSRecommender(itemManager, itemCFManager, userEventFilter_, cartManager);
    recommenders_[BASED_ON_RANDOM] = new BORRecommender(itemManager, userEventFilter_, itemIdGenerator);
}

RecommenderFactory::~RecommenderFactory()
{
    for (std::vector<Recommender*>::iterator it = recommenders_.begin();
        it != recommenders_.end(); ++it)
    {
        delete *it;
    }
    recommenders_.clear();
}

RecommendType RecommenderFactory::getRecommendType(const std::string& typeStr) const
{
    std::string upper = boost::algorithm::to_upper_copy(typeStr);

    RecTypeMap::const_iterator it = recTypeMap_.find(upper);
    if (it != recTypeMap_.end())
        return it->second;

    return RECOMMEND_TYPE_NUM;
}

Recommender* RecommenderFactory::getRecommender(RecommendType type)
{
    if (type != RECOMMEND_TYPE_NUM)
        return recommenders_[type];

    LOG(ERROR) << "RecommendType " << type << " is not supported yet";
    return NULL;
}

TIBRecommender* RecommenderFactory::getTIBRecommender()
{
    return &tibRecommender_;
}

} // namespace sf1r
