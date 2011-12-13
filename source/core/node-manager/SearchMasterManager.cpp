#include "SearchMasterManager.h"
#include "SearchNodeManager.h"
#include "ZooKeeperManager.h"

namespace sf1r
{

SearchMasterManager::SearchMasterManager()
{
    CLASSNAME = "[SearchMasterManager]";
}

bool SearchMasterManager::init()
{
    // initialize zookeeper client
    zookeeper_ = ZooKeeperManager::get()->createClient(this);

    if (!zookeeper_)
        return false;

    // initialize topology info
    topology_.clusterId_ = SearchNodeManager::get()->getDSTopologyConfig().clusterId_;
    topology_.nodeNum_ =  SearchNodeManager::get()->getDSTopologyConfig().nodeNum_;
    topology_.shardNum_ =  SearchNodeManager::get()->getDSTopologyConfig().shardNum_;

    curNodeInfo_ = SearchNodeManager::get()->getNodeInfo();

    return true;
}


}
