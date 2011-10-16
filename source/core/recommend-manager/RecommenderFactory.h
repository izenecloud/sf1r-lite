/**
 * @file RecommenderFactory.h
 * @brief given a recommend type, return a Recommender instance
 * @author Jun Jiang
 * @date 2011-10-14
 */

#ifndef RECOMMENDER_FACTORY_H
#define RECOMMENDER_FACTORY_H

#include "Recommender.h"
#include "UserEventFilter.h"
#include "FBTRecommender.h"
#include "VAVRecommender.h"
#include "BABRecommender.h"
#include "BOBRecommender.h"
#include "BOSRecommender.h"
#include "BOERecommender.h"
#include "TIBRecommender.h"

namespace sf1r
{
class ItemManager;
class VisitManager;
class PurchaseManager;
class CartManager;
class OrderManager;
class EventManager;

class RecommenderFactory
{
public:
    RecommenderFactory(
        ItemManager& itemManager,
        VisitManager& visitManager,
        PurchaseManager& purchaseManager,
        CartManager& cartManager,
        OrderManager& orderManager,
        EventManager& eventManager,
        CoVisitManager& coVisitManager,
        ItemCFManager& itemCFManager
    );

    Recommender* getRecommender(RecommendType type);

    TIBRecommender* getTIBRecommender();

private:
    UserEventFilter userEventFilter_;
    FBTRecommender fbtRecommender_;
    VAVRecommender vavRecommender_;
    BABRecommender babRecommender_;
    BOBRecommender bobRecommender_;
    BOSRecommender bosRecommender_;
    BOERecommender boeRecommender_;
    TIBRecommender tibRecommender_;
};

} // namespace sf1r

#endif // RECOMMENDER_FACTORY_H
