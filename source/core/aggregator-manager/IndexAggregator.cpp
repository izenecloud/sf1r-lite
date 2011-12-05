#include "IndexAggregator.h"

#include <node-manager/NodeManager.h>
#include <node-manager/MasterNodeManager.h>

#include <node-manager/sharding/ScdSharding.h>
#include <node-manager/sharding/ShardingStrategy.h>
#include <node-manager/sharding/ScdDispatcher.h>

namespace sf1r{

bool IndexAggregator::ScdDispatch(
        unsigned int numdoc,
        const std::string& collectionName,
        const std::string& scdPath)
{
    bool ret = false;

    // xxx config
    ShardingConfig cfg;
    cfg.setShardNum(NodeManagerSingleton::get()->getShardNum());
    cfg.addShardKey("Url");
    cfg.addShardKey("Title");
    cfg.addShardKey("DOCID");

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
