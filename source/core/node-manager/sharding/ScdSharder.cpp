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

    // set sharding key values
    setShardKeyValues(scdDoc);

    return shardingStrategy_->sharding(ShardFieldList_, shardingConfig_);
}

bool ScdSharder::init(ShardingConfig& shardingConfig)
{
    shardingConfig_ = shardingConfig;

    return initShardingStrategy();
}

bool ScdSharder::initShardingStrategy()
{
    if (shardingConfig_.getShardStrategy() == ShardingConfig::SHARDING_HASH)
    {
        shardingStrategy_.reset(new HashShardingStrategy);
    }
    else
    {
        LOG(ERROR) << "ShardingStrategy not specified!";
        return false;
    }

    return true;
}

void ScdSharder::setShardKeyValues(SCDDoc& scdDoc)
{
    ShardFieldList_.clear();

    std::string docidVal;
    SCDDoc::iterator propertyIter;
    for (propertyIter = scdDoc.begin(); propertyIter != scdDoc.end(); propertyIter++)
    {
        const std::string& propertyName = propertyIter->first;
        std::string propertyValue = propstr_to_str(propertyIter->second);

        if (shardingConfig_.hasShardKey(propertyName))
        {
            ShardFieldList_.push_back(std::make_pair(propertyName, propertyValue));
            //std::cout<< "ShardFieldList_ k-v:" <<propertyName<<" - "<<propertyValue<<std::endl;
        }

        if (propertyName == "DOCID")
            docidVal = propertyValue;
    }

    if (ShardFieldList_.empty())
    {
        // set DOCID as default shard key if missing shard keys, xxx
        ShardFieldList_.push_back(std::make_pair("DOCID", docidVal));
        LOG(WARNING) << "WARN: miss shard keys (properties) in doc: " << docidVal;
    }
}
