/**
 * @file BABRecommender.h
 * @brief recommend "bought also bought" items
 * @author Jun Jiang
 * @date 2011-10-12
 */

#ifndef BAB_RECOMMENDER_H
#define BAB_RECOMMENDER_H

#include "RecTypes.h"
#include "Recommender.h"

namespace sf1r
{

class BABRecommender : public Recommender
{
public:
    BABRecommender(ItemManager& itemManager, ItemCFManager& itemCFManager);

    bool recommend(RecommendParam& param, std::vector<RecommendItem>& recItemVec);

private:
    ItemCFManager& itemCFManager_;
};

} // namespace sf1r

#endif // BAB_RECOMMENDER_H
