#include "IndexAggregator.h"

#include <node-manager/NodeManager.h>
#include <node-manager/MasterNodeManager.h>

#include <node-manager/sharding/ScdSharding.h>
#include <node-manager/sharding/ShardingStrategy.h>
#include <node-manager/sharding/ScdDispatcher.h>

#include <glog/logging.h>

namespace sf1r{

bool IndexAggregator::ScdDispatch(
        unsigned int numdoc,
        const std::string& collectionName,
        const std::string& scdPath,
        const std::vector<std::string>& shardKeyList)
{
    bool ret = false;

    ShardingConfig cfg;
    cfg.setShardNum(NodeManagerSingleton::get()->getShardNum());
    size_t i = 0;
    for ( ; i < shardKeyList.size(); i++)
    {
        cfg.addShardKey(shardKeyList[i]);
        std::cout << "Shard Key: " << shardKeyList[i] << std::endl;
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
            MasterNodeManagerSingleton::get()->getAggregatorConfig(),
            collectionName);

    // do dispatch
    ret = scdDispatcher->dispatch(scdPath, numdoc);

    delete shardingStrategy;
    delete scdDispatcher;

    return ret;
}

}
