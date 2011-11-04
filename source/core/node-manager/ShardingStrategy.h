/**
 * @file ShardingStrategy.h
 * @author Zhongxia Li
 * @date Nov 3, 2011
 * @brief Strategies (patterns or algorithms) for generating shard id
 */
#ifndef SHARDING_STRATEGY_H_
#define SHARDING_STRATEGY_H_

#include <stdint.h>

#include <vector>

#include <util/ustring/UString.h>

namespace sf1r{

typedef uint32_t shardid_t; // xxx, shard id start with 1 (1, ..., n)

/// TODO
class ShardingStrategy
{
public:
    typedef std::vector<std::pair<izenelib::util::UString, izenelib::util::UString> > ShardFieldListT;

    struct ShardingParams
    {
        unsigned int shardNum_;
    };

    virtual shardid_t sharding(ShardFieldListT& shardFieldList, ShardingParams& shardingParams) = 0;
};

class HashShardingStrategy : public ShardingStrategy
{
public:
    virtual shardid_t sharding(ShardFieldListT& shardFieldList, ShardingParams& shardingParams);
};

}

#endif /* SHARDING_STRATEGY_H_ */
