/**
 * @file ShardingStrategy.h
 * @author Zhongxia Li
 * @date Nov 3, 2011
 * @brief Strategies (patterns or algorithms) for generating shard id
 */
#ifndef SHARDING_STRATEGY_H_
#define SHARDING_STRATEGY_H_

#include "ShardingConfig.h"

#include <util/ustring/UString.h>

#include <boost/uuid/sha1.hpp>

#include <vector>
#include <map>

#include <stdint.h>

namespace sf1r{

class ShardingStrategy
{
public:
    typedef std::map<std::string, std::string> ShardFieldListT;
    typedef std::vector<shardid_t>  ShardIDListT;

    virtual shardid_t sharding_for_write(const ShardFieldListT& shardFieldList, const ShardingConfig& shardingConfig) = 0;
    virtual ShardIDListT sharding_for_get(const ShardFieldListT& shardFieldList, const ShardingConfig& shardingConfig) = 0;

    virtual ~ShardingStrategy(){}
};

class HashShardingStrategy : public ShardingStrategy
{
public:
    virtual ShardIDListT sharding_for_get(const ShardFieldListT& shardFieldList, const ShardingConfig& shardingConfig);
    virtual shardid_t sharding_for_write(const ShardFieldListT& shardFieldList, const ShardingConfig& shardingConfig);
};

}

#endif /* SHARDING_STRATEGY_H_ */
