#include "SearchMasterManager.h"
#include "SearchNodeManager.h"


namespace sf1r
{

SearchMasterManager::SearchMasterManager()
{
    CLASSNAME = "[SearchMasterManager]";
}

void SearchMasterManager::init()
{
    // initialize zookeeper
    initZooKeeper(
            SearchNodeManager::get()->getDSUtilConfig().zkConfig_.zkHosts_,
            SearchNodeManager::get()->getDSUtilConfig().zkConfig_.zkRecvTimeout_);

    // initialize topology info
    topology_.clusterId_ = SearchNodeManager::get()->getDSTopologyConfig().clusterId_;
    topology_.nodeNum_ =  SearchNodeManager::get()->getDSTopologyConfig().nodeNum_;
    topology_.shardNum_ =  SearchNodeManager::get()->getDSTopologyConfig().shardNum_;

    curNodeInfo_ = SearchNodeManager::get()->getNodeInfo();
}


}
