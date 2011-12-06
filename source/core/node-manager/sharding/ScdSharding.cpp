#include "ScdSharding.h"

#include <boost/assert.hpp>

using namespace sf1r;

ScdSharding::ScdSharding(ShardingConfig& shardingConfig, ShardingStrategy* shardingStrategy)
:shardingConfig_(shardingConfig), shardingStrategy_(shardingStrategy)
{
    BOOST_ASSERT(shardingStrategy_);
}

shardid_t ScdSharding::sharding(SCDDoc& scdDoc)
{
    // set sharding key values
    setShardKeyValues(scdDoc);

    // set sharding parameters
    ShardingStrategy::ShardingParams shardingParams;
    shardingParams.shardNum_ = shardingConfig_.getShardNum();

    return shardingStrategy_->sharding(ShardFieldList_, shardingParams);
}

void ScdSharding::setShardKeyValues(SCDDoc& scdDoc)
{
    ShardFieldList_.clear();

    std::string docidVal;
    SCDDoc::iterator propertyIter;
    for (propertyIter = scdDoc.begin(); propertyIter != scdDoc.end(); propertyIter++)
    {
        std::string propertyName;
        std::string propertyValue;
        (*propertyIter).first.convertString(propertyName, izenelib::util::UString::UTF_8);
        (*propertyIter).second.convertString(propertyValue, izenelib::util::UString::UTF_8);

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
        // set DOCID as default shard key if miss shard keys, xxx
        ShardFieldList_.push_back(std::make_pair("DOCID", docidVal));
        std::cerr<<"WARN: miss shard keys (properties) in doc: "<<docidVal<<std::endl;
    }
}
