#include "SearchMasterManager.h"
#include "SearchNodeManager.h"
#include "SuperNodeManager.h"
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
    topologyPath_ = ZooKeeperNamespace::getSearchTopologyPath();
    serverParentPath_ = ZooKeeperNamespace::getSearchServerParentPath();
    serverPath_ = ZooKeeperNamespace::getSearchServerPath();
    zookeeper_ = ZooKeeperManager::get()->createClient(this);

    if (!zookeeper_)
        return false;

    // initialize topology info
    topology_.clusterId_ = SuperNodeManager::get()->getCommonConfig().clusterId_;
    topology_.nodeNum_ =  SearchNodeManager::get()->getDSTopologyConfig().nodeNum_;
    topology_.shardNum_ =  SearchNodeManager::get()->getDSTopologyConfig().curSF1Node_.masterAgent_.shardNum_;
    curNodeInfo_ = SearchNodeManager::get()->getNodeInfo();

    return true;
}


}
