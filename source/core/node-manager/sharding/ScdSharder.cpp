#include "ScdSharder.h"
#include <common/PropertyValue.h>

#include <boost/assert.hpp>

#include <glog/logging.h>

using namespace sf1r;

ScdSharder::ScdSharder()
{
}

shardid_t ScdSharder::sharding(SCDDoc& scdDoc)
{
    if (!shardingStrategy_)
    {
        LOG(ERROR) << "shardingStrategy not initialized!";
        return 0;
    }

    static ShardingStrategy::ShardFieldListT shard_fieldlist;
    ShardingStrategy::ShardFieldListT().swap(shard_fieldlist);
    // set sharding key values
    setShardKeyValues(scdDoc, shard_fieldlist);

    return shardingStrategy_->sharding_for_write(shard_fieldlist, shardingConfig_);
}

bool ScdSharder::init(ShardingConfig& shardingConfig)
{
    shardingConfig_ = shardingConfig;

    return initShardingStrategy();
}

bool ScdSharder::initShardingStrategy()
{
    shardingStrategy_.reset(new HashShardingStrategy);
    return true;
}

void ScdSharder::setShardKeyValues(SCDDoc& scdDoc, ShardingStrategy::ShardFieldListT& shard_fieldlist)
{
    SCDDoc::iterator propertyIter;
    for (propertyIter = scdDoc.begin(); propertyIter != scdDoc.end(); propertyIter++)
    {
        const std::string& propertyName = propertyIter->first;

        if (shardingConfig_.isUniqueShardKey(propertyName))
        {
            const std::string propertyValue = propstr_to_str(propertyIter->second);
            shard_fieldlist[propertyName] = propertyValue;
            //std::cout<< "shard k-v:" <<propertyName<<" - "<<propertyValue<<std::endl;
            break;
        }
    }

    if (shard_fieldlist.empty())
    {
        LOG(WARNING) << "WARN: miss shard keys (properties) in doc";
    }
}
