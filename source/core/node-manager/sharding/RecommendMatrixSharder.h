/**
 * @file RecommendMatrixSharder.h
 * @brief a sharder class, used to test whether a matrix row
 * should be stored on current node.
 * @author Jun Jiang
 * @date 2012-04-15
 */

#ifndef SF1R_RECOMMEND_MATRIX_SHARDER_H
#define SF1R_RECOMMEND_MATRIX_SHARDER_H

#include "RecommendShardStrategy.h"
#include <idmlib/resys/MatrixSharder.h>

namespace sf1r
{

class RecommendMatrixSharder : public idmlib::recommender::MatrixSharder
{
public:
    RecommendMatrixSharder(
        shardid_t curShardId,
        RecommendShardStrategy* shardStrategy
    )
        : curShardId_(curShardId)
        , shardStrategy_(shardStrategy)
    {}

    virtual bool testRow(uint32_t rowId)
    {
        return shardStrategy_->getShardId(rowId) == curShardId_;
    }

private:
    shardid_t curShardId_;
    RecommendShardStrategy* shardStrategy_;
};

} // namespace sf1r

#endif // SF1R_RECOMMEND_MATRIX_SHARDER_H
