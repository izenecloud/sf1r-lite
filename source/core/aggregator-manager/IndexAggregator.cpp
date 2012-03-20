#include "IndexAggregator.h"

#include <node-manager/SearchNodeManager.h>
#include <node-manager/SearchMasterManager.h>

#include <node-manager/sharding/ScdSharder.h>
#include <node-manager/sharding/ShardingStrategy.h>
#include <node-manager/sharding/ScdDispatcher.h>

#include <glog/logging.h>

namespace sf1r{


bool IndexAggregator::distributedIndex(
        unsigned int numdoc,
        const std::string& collectionName,
        const std::string& masterScdPath,
        const std::vector<std::string>& shardKeyList)
{
    // 1. dispatching scd to multiple nodes
    boost::shared_ptr<ScdSharder> scdSharder;
    if (!createScdSharder(scdSharder, shardKeyList))
    {
        return false;
    }

    boost::shared_ptr<ScdDispatcher> scdDispatcher(new BatchScdDispatcher(scdSharder, collectionName));
    if(scdDispatcher->dispatch(masterScdPath, numdoc))
    {
        return false;
    }

   // 2. send index request to multiple nodes
   bool ret = true;
   LOG(INFO) << "start distributed indexing";
   this->distributeRequest(collectionName, "index", numdoc, ret);

   if (ret)
   {
       // backup master
   }

   return ret;
}

bool IndexAggregator::createScdSharder(
        boost::shared_ptr<ScdSharder>& scdSharder,
        const std::vector<std::string>& shardKeyList)
{
    if (shardKeyList.empty())
    {
        LOG(ERROR) << "No shard key!";
        return false;
    }

    // sharding configuration
    ShardingConfig cfg;
    cfg.setShardNum(SearchNodeManager::get()->getShardNum());
    for (size_t i = 0; i < shardKeyList.size(); i++)
    {
        cfg.addShardKey(shardKeyList[i]);
        LOG(INFO) << "Shard Key: " << shardKeyList[i];
    }
    cfg.setShardStrategy(ShardingConfig::SHARDING_HASH); // use proper strategy

    scdSharder.reset(new ScdSharder);

    if (scdSharder && scdSharder->init(cfg))
        return true;
    else
        return false;
}

}
