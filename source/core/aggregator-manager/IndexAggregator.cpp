#include "IndexAggregator.h"

#include <node-manager/SearchNodeManager.h>
#include <node-manager/SearchMasterManager.h>

#include <node-manager/sharding/ScdSharding.h>
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
    // scd sharding & dispatching
    if (!this->ScdDispatch(numdoc, collectionName, masterScdPath, shardKeyList))
    {
       return false;
    }

   // backup master

   // distributed indexing request
   LOG(INFO) << "start distributed indexing";
   bool ret = true;
   this->setDebug(true);
   this->distributeRequest(collectionName, "index", numdoc, ret);
   return true;
}

bool IndexAggregator::ScdDispatch(
        unsigned int numdoc,
        const std::string& collectionName,
        const std::string& masterScdPath,
        const std::vector<std::string>& shardKeyList)
{
    bool ret = false;

    ShardingConfig cfg;
    cfg.setShardNum(SearchNodeManager::get()->getShardNum());
    size_t i = 0;
    for ( ; i < shardKeyList.size(); i++)
    {
        cfg.addShardKey(shardKeyList[i]);
        LOG(INFO) << "Shard Key: " << shardKeyList[i];
    }
    if (i == 0)
    {
        LOG(ERROR) << "No shard key!";
        return ret;
    }

    // create scd sharding strategy
    ShardingStrategy* shardingStrategy = new HashShardingStrategy;
    ScdSharding scdSharding(cfg, shardingStrategy);

    // create scd dispatcher
    ScdDispatcher* scdDispatcher = new BatchScdDispatcher(
            &scdSharding,
            collectionName);

    // do dispatch
    ret = scdDispatcher->dispatch(masterScdPath, numdoc);

    delete shardingStrategy;
    delete scdDispatcher;

    return ret;
}

}
