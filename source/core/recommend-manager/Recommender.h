/**
 * @file Recommender.h
 * @brief interface class to recommend items
 * @author Jun Jiang
 * @date 2011-10-14
 */

#ifndef RECOMMENDER_H
#define RECOMMENDER_H

#include "ItemManager.h"
#include "RecommendParam.h"
#include "RecommendItem.h"

#include <vector>

namespace sf1r
{

class Recommender
{
public:
    Recommender(ItemManager& itemManager)
        : itemManager_(itemManager)
    {}

    ~Recommender() {}

    virtual bool recommend(RecommendParam& param, std::vector<RecommendItem>& recItemVec) = 0;

protected:
    ItemManager& itemManager_;
};

} // namespace sf1r

#endif // RECOMMENDER_H
