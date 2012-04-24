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

#include <stdint.h>

namespace sf1r{

typedef uint32_t shardid_t; // xxx, shard id start with 1 (1, ..., n)

class ShardingStrategy
{
public:
    typedef std::vector<std::pair<std::string, std::string> > ShardFieldListT;

    struct ShardingParams
    {
        unsigned int shardNum_;
    };

    virtual shardid_t sharding(ShardFieldListT& shardFieldList, ShardingConfig& shardingConfig) = 0;

    virtual ~ShardingStrategy(){}
};

class HashShardingStrategy : public ShardingStrategy
{
public:
    virtual shardid_t sharding(ShardFieldListT& shardFieldList, ShardingConfig& shardingConfig);

private:
    uint64_t hashmd5(const char* data, unsigned long len);

    uint64_t hashsha(const char* data, unsigned long len);

private:
    boost::uuids::detail::sha1 sha1_;
};

}

#endif /* SHARDING_STRATEGY_H_ */
