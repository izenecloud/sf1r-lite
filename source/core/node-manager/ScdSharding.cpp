#include "ScdSharding.h"

using namespace sf1r;

ScdSharding::ScdSharding(ShardingConfig& shardingConfig, ShardingStrategy* shardingStrategy)
:shardingConfig_(shardingConfig), shardingStrategy_(shardingStrategy)
{
}

shardid_t ScdSharding::sharding(SCDDoc& scdDoc)
{
    if (!shardingStrategy_)
        return 0;

    // Initialize shardFieldList_
    makeShardFieldList(scdDoc);

    ShardingStrategy::ShardingParams shardingParams;
    shardingParams.shardNum_ = shardingConfig_.getShardNum();

    return shardingStrategy_->sharding(shardFieldList_, shardingParams);
}

void ScdSharding::makeShardFieldList(SCDDoc& scdDoc)
{
    shardFieldList_.clear(); // xxx init

    SCDDoc::iterator propertyIter;
    for (propertyIter = scdDoc.begin(); propertyIter != scdDoc.end(); propertyIter++)
    {
        std::string shardKey;
        (*propertyIter).first.convertString(shardKey, izenelib::util::UString::UTF_8);
        if (shardingConfig_.hasShardKey(shardKey))
        {
            shardFieldList_.push_back(std::make_pair((*propertyIter).first, (*propertyIter).second));
            //std::cout<<"makeShardFieldList: "<<shardKey<<std::endl;
        }
    }
}
