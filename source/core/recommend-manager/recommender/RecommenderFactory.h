/**
 * @file RecommenderFactory.h
 * @brief given a recommend type, return a Recommender instance
 * @author Jun Jiang
 * @date 2011-10-14
 */

#ifndef RECOMMENDER_FACTORY_H
#define RECOMMENDER_FACTORY_H

#include "../common/RecTypes.h"
#include "UserEventFilter.h"
#include "TIBRecommender.h"

#include <map>
#include <string>

namespace sf1r
{
class ItemIdGenerator;
class VisitManager;
class PurchaseManager;
class CartManager;
class OrderManager;
class EventManager;
class RateManager;
class Recommender;
class GetRecommendBase;

class RecommenderFactory
{
public:
    RecommenderFactory(
        ItemIdGenerator& itemIdGenerator,
        VisitManager& visitManager,
        PurchaseManager& purchaseManager,
        CartManager& cartManager,
        OrderManager& orderManager,
        EventManager& eventManager,
        RateManager& rateManager,
        QueryClickCounter& queryPurchaseCounter,
        GetRecommendBase& getRecommendBase
    );

    ~RecommenderFactory();

    /**
     * @return type id, if no type id is found, @c RECOMMEND_TYPE_NUM is returned
     */
    RecommendType getRecommendType(const std::string& typeStr) const;

    Recommender* getRecommender(RecommendType type);

    TIBRecommender* getTIBRecommender();

private:
    UserEventFilter userEventFilter_;
    TIBRecommender tibRecommender_;

    /** key: recommendation type string */
    typedef std::map<std::string, RecommendType> RecTypeMap;
    RecTypeMap recTypeMap_;

    /** key: RecommendType */
    std::vector<Recommender*> recommenders_;
};

} // namespace sf1r

#endif // RECOMMENDER_FACTORY_H
