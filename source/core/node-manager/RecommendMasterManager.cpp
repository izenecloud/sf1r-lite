#include "RecommendMasterManager.h"
#include "RecommendNodeManager.h"
#include "SuperNodeManager.h"
#include "ZooKeeperManager.h"

namespace sf1r
{

RecommendMasterManager::RecommendMasterManager()
{
    CLASSNAME = "[RecommendMasterManager]";
}

bool RecommendMasterManager::init()
{
    // initialize zookeeper client
    topologyPath_ = ZooKeeperNamespace::getRecommendTopologyPath();
    serverParentPath_ = ZooKeeperNamespace::getRecommendServerParentPath();
    serverPath_ = ZooKeeperNamespace::getRecommendServerPath();
    zookeeper_ = ZooKeeperManager::get()->createClient(this);

    if (!zookeeper_)
        return false;

    // initialize topology info
    topology_.clusterId_ = SuperNodeManager::get()->getCommonConfig().clusterId_;
    topology_.nodeNum_ =  RecommendNodeManager::get()->getDSTopologyConfig().nodeNum_;
    topology_.shardNum_ =  RecommendNodeManager::get()->getDSTopologyConfig().shardNum_;
    curNodeInfo_ = RecommendNodeManager::get()->getNodeInfo();

    return true;
}

}


