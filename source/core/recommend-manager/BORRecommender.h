/**
 * @file BORRecommender.h
 * @brief recommend items "based on random"
 * @author Jun Jiang
 * @date 2011-11-30
 */

#ifndef BOR_RECOMMENDER_H
#define BOR_RECOMMENDER_H

#include "Recommender.h"
#include "RandGenerator.h"
#include "RecTypes.h"

namespace sf1r
{

class UserEventFilter;

class BORRecommender : public Recommender
{
public:
    BORRecommender(
        ItemManager& itemManager,
        const UserEventFilter& userEventFilter
    );

protected:
    virtual bool recommendImpl_(
        RecommendParam& param,
        ItemFilter& filter,
        std::vector<RecommendItem>& recItemVec
    );

private:
    RandGenerator<itemid_t> rand_;
    const UserEventFilter& userEventFilter_;
};

} // namespace sf1r

#endif // BOR_RECOMMENDER_H
