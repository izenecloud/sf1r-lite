/**
 * @file Recommender.h
 * @brief interface class to recommend items
 * @author Jun Jiang
 * @date 2011-10-14
 */

#ifndef RECOMMENDER_H
#define RECOMMENDER_H

#include <vector>

namespace sf1r
{
class ItemManager;
class ItemFilter;
struct RecommendParam;
struct RecommendItem;

class Recommender
{
public:
    Recommender(ItemManager& itemManager);

    virtual ~Recommender() {}

    bool recommend(
        RecommendParam& param,
        std::vector<RecommendItem>& recItemVec
    );

protected:
    virtual bool recommendImpl_(
        RecommendParam& param,
        ItemFilter& filter,
        std::vector<RecommendItem>& recItemVec
    ) = 0;

protected:
    ItemManager& itemManager_;
};

} // namespace sf1r

#endif // RECOMMENDER_H
