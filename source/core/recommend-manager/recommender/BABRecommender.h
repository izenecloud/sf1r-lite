/**
 * @file BABRecommender.h
 * @brief recommend "bought also bought" items
 * @author Jun Jiang
 * @date 2011-10-12
 */

#ifndef BAB_RECOMMENDER_H
#define BAB_RECOMMENDER_H

#include "ItemCFRecommender.h"

namespace sf1r
{

class BABRecommender : public ItemCFRecommender
{
public:
    BABRecommender(ItemCFManager& itemCFManager);

protected:
    virtual bool recommendImpl_(
        RecommendParam& param,
        std::vector<RecommendItem>& recItemVec
    );
};

} // namespace sf1r

#endif // BAB_RECOMMENDER_H
