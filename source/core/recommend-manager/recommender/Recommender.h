/**
 * @file Recommender.h
 * @brief interface class to recommend items
 * @author Jun Jiang
 * @date 2011-10-14
 */

#ifndef RECOMMENDER_H
#define RECOMMENDER_H

#include "../common/RecommendParam.h"
#include "../common/RecommendItem.h"

#include <vector>

namespace sf1r
{

class Recommender
{
public:
    virtual ~Recommender() {}

    bool recommend(
        RecommendParam& param,
        std::vector<RecommendItem>& recItemVec
    );

protected:
    virtual bool recommendImpl_(
        RecommendParam& param,
        std::vector<RecommendItem>& recItemVec
    ) = 0;
};

} // namespace sf1r

#endif // RECOMMENDER_H
