/**
 * @file /sf1r-engine/source/core/node-manager/sharding/ShardingConfig.h
 * @author Zhongxia Li
 * @date Mar 19, 2012
 * @brief  
 */

#ifndef SHARDING_CONFIG_H_
#define SHARDING_CONFIG_H_

#include <set>
#include <vector>

#include <boost/algorithm/string.hpp>

namespace sf1r
{

class ShardingConfig
{
public:
    enum ShardingStrategyType
    {
        SHARDING_HASH
    };

public:
    ShardingConfig()
        : shardNum_(0)
        , shardStrategyType_(SHARDING_HASH)
    {}

    void setShardNum(unsigned int shardNum) { shardNum_ = shardNum; }

    unsigned int getShardNum() { return shardNum_; }

    void addShardKey(const std::string& shardKey)
    {
        shardKeys_.insert(shardKey);
    }

    std::set<std::string>& getShardKeys()
    {
        return shardKeys_;
    }

    bool hasShardKey(const std::string& shardKey)
    {
        std::set<std::string>::iterator it;
        for (it = shardKeys_.begin(); it != shardKeys_.end(); it++)
        {
            // case insensitive
            if (boost::iequals(*it, shardKey))
                return true;
        }
        return false;
    }

    void clearShardKeys() { shardKeys_.clear(); }

    bool isShardKeysEmpty() { return shardKeys_.empty(); }

    void setShardStrategy(ShardingStrategyType shardStrategyType)
    {
        shardStrategyType_ = shardStrategyType;
    }

    ShardingStrategyType getShardStrategy()
    {
        return shardStrategyType_;
    }

public:
    unsigned int shardNum_;
    std::vector<uint32_t> shardidList_;
    std::set<std::string> shardKeys_;
    ShardingStrategyType shardStrategyType_;
};

}

#endif /* SHARDING_CONFIG_H_ */
