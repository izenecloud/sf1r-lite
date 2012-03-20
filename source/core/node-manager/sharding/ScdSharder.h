/**
 * @file ScdPartitioner.h
 * @author Zhongxia Li
 * @date Nov 3, 2011
 * @brief sharding (partitioning) a scd document to a shard.
 */
#ifndef SCD_SHARDER_H_
#define SCD_SHARDER_H_

#include <set>

#include "ShardingStrategy.h"
#include "ShardingConfig.h"

#include <common/ScdParser.h>

#include <boost/shared_ptr.hpp>

namespace sf1r{


class ScdSharder
{
public:
    ScdSharder();

    bool init(ShardingConfig& shardingConfig);

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
    bool initShardingStrategy();

    void setShardKeyValues(SCDDoc& scdDoc);

private:
    ShardingConfig shardingConfig_;
    boost::shared_ptr<ShardingStrategy> shardingStrategy_;
    ShardingStrategy::ShardFieldListT ShardFieldList_;
};

}

#endif /* SCD_SHARDER_H_ */
