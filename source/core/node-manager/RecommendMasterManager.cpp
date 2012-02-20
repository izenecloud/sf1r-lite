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

bool RecommendMasterManager::getSearchMasterAddress(std::string& host, uint32_t& port)
{
    if (!zookeeper_ || !zookeeper_->isConnected())
        return false;

    std::string searchMastersPath = ZooKeeperNamespace::getSearchServerParentPath();
    std::vector<std::string> children;
    zookeeper_->getZNodeChildren(searchMastersPath, children);

    if (children.size() > 0)
    {
        std::string searchMasterPath = children[0];

        std::string data;
        if (zookeeper_->getZNodeData(searchMasterPath, data))
        {
            ZNode znode;
            znode.loadKvString(data);

            host = znode.getStrValue(ZNode::KEY_HOST);
            port = znode.getUInt32Value(ZNode::KEY_MASTER_PORT);
            return true;
        }
    }

    return false;
}

}


