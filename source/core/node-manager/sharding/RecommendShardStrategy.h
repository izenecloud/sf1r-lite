/**
 * @file RecommendShardStrategy.h
 * @brief the strategy of items sharding in recommendation
 * @author Jun Jiang
 * @date 2012-04-15
 */

#ifndef SF1R_RECOMMEND_SHARD_STRATEGY_H
#define SF1R_RECOMMEND_SHARD_STRATEGY_H

#include <node-manager/Sf1rTopology.h>
#include <recommend-manager/common/RecTypes.h>
#include <boost/lexical_cast.hpp>

namespace sf1r
{

/**
 * the interface of sharding strategy in recommendation.
 */
class RecommendShardStrategy
{
public:
    RecommendShardStrategy(std::vector<shardid_t> shardids) : shardids_(shardids) {}

    virtual ~RecommendShardStrategy() {}

    shardid_t getShardNum() const { return shardids_.size(); }

    /**
     * get shard id for @p itemId.
     * @param itemId item id
     * @return the shard id, its range should be [1, @c shardNum_]
     */
    virtual shardid_t getShardId(itemid_t itemId) = 0;
    virtual shardid_t shardingForUser(const std::string& user_id) = 0;

protected:
    std::vector<shardid_t> shardids_;
};

/**
 * a sharding strategy using modular arithmetic.
 */
class RecommendShardMod : public RecommendShardStrategy
{
public:
    RecommendShardMod(std::vector<shardid_t> shardids) : RecommendShardStrategy(shardids) {}

    virtual shardid_t getShardId(itemid_t itemId)
    {
        return shardids_[itemId % shardids_.size()];
    }
    virtual shardid_t shardingForUser(const std::string& user_id)
    {
        return shardids_[boost::lexical_cast<uint32_t>(user_id) % shardids_.size()];
    }
};

} // namespace sf1r

#endif // SF1R_RECOMMEND_SHARD_STRATEGY_H
