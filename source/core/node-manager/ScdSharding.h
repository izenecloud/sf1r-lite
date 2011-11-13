/**
 * @file ScdPartitioner.h
 * @author Zhongxia Li
 * @date Nov 3, 2011
 * @brief sharding(partitioning) a scd document to one of the shards in cluster.
 */
#ifndef SCD_SHARDING_H_
#define SCD_SHARDING_H_

#include <set>

#include "ShardingStrategy.h"

#include <common/ScdParser.h>

#include <boost/algorithm/string.hpp>

namespace sf1r{

class ScdShardingException : public std::exception
{
public:
    ScdShardingException(const std::string& msg)
    :msg_(msg) {}

    ~ScdShardingException() throw() {}

    const char *what() const throw()
    {
        return msg_.c_str();
    }

private:
    std::string msg_;
};

class ShardingConfig
{
public:
    ShardingConfig():shardNum_(0) {}

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

private:
    unsigned int shardNum_;
    std::set<std::string> shardKeys_;
};

class ScdSharding
{
public:
    ScdSharding(ShardingConfig& shardingConfig, ShardingStrategy* shardingStrategy);

public:
    /**
     * For a given SCD Document, sharding to
     * @param scdDoc
     * @return shard id (0 is invalid id for a shard)
     */
    shardid_t sharding(SCDDoc& scdDoc);

    unsigned int getShardNum() { return shardingConfig_.getShardNum(); }
    ShardingConfig& getShardingConfig() { return shardingConfig_; }

    /// xxx
    unsigned int getMinShardID() { return 1; }
    unsigned int getMaxShardID() { return shardingConfig_.getShardNum(); }

private:
    void setShardKeyValues(SCDDoc& scdDoc);

private:
    ShardingConfig shardingConfig_;
    ShardingStrategy* shardingStrategy_;
    ShardingStrategy::ShardFieldListT ShardFieldList_;
};

}

#endif /* SCD_SHARDING_H_ */
