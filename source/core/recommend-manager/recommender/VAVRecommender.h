/**
 * @file VAVRecommender.h
 * @brief recommend "viewed also view" items
 * @author Jun Jiang
 * @date 2011-10-12
 */

#ifndef VAV_RECOMMENDER_H
#define VAV_RECOMMENDER_H

#include "Recommender.h"

namespace sf1r
{

class VAVRecommender : public Recommender
{
public:
    VAVRecommender(CoVisitManager& coVisitManager);

protected:
    virtual bool recommendImpl_(
        RecommendParam& param,
        std::vector<RecommendItem>& recItemVec
    );

private:
    CoVisitManager& coVisitManager_;
};

} // namespace sf1r

#endif // VAV_RECOMMENDER_H
