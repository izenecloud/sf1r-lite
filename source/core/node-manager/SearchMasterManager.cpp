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
            SearchNodeManagerSingleton::get()->getDSUtilConfig().zkConfig_.zkHosts_,
            SearchNodeManagerSingleton::get()->getDSUtilConfig().zkConfig_.zkRecvTimeout_);

    // initialize topology info
    topology_.clusterId_ = SearchNodeManagerSingleton::get()->getDSTopologyConfig().clusterId_;
    topology_.nodeNum_ =  SearchNodeManagerSingleton::get()->getDSTopologyConfig().nodeNum_;
    topology_.shardNum_ =  SearchNodeManagerSingleton::get()->getDSTopologyConfig().shardNum_;

    curNodeInfo_ = SearchNodeManagerSingleton::get()->getNodeInfo();
}


}
