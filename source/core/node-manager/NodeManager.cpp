#include "NodeManager.h"


using namespace sf1r;

NodeManager::NodeManager()
{

}

void NodeManager::initZooKeeper(const std::string& zkHosts, const int recvTimeout)
{
    zookeeper_.reset(new ZooKeeper(zkHosts, recvTimeout));
    zookeeper_->registerEventHandler(this);
}

void NodeManager::registerNode(nodeid_t nodeId, mirrorid_t mirrorId, const std::string& data)
{
    std::string zpath = NodeUtil::getNodePath(mirrorId, nodeId);

    if (zookeeper_->isConnected()) {
        // xxx check existence of path
        zookeeper_->createZNode(zpath, data);
    }
    else {
        registerTaskList_.push_back(std::make_pair(zpath, data));
    }
}

void NodeManager::registerNodeMaster(nodeid_t nodeId, mirrorid_t mirrorId, const std::string& data)
{
    std::string zpath = NodeUtil::getNodePath(mirrorId, nodeId);

    if (zookeeper_->isConnected()) {
        zookeeper_->createZNode(zpath, data, ZooKeeper::ZNODE_EPHEMERAL);
    }
    else {
        registerTaskList_.push_back(std::make_pair(zpath, data));
    }
}

void NodeManager::process(ZooKeeperEvent& zkEvent)
{
    if (zookeeper_->isConnected())
    {
        doProcess();
    }
}

void NodeManager::doProcess()
{
    std::vector<std::pair<std::string, std::string> >::iterator it;
    for (it = registerTaskList_.begin(); it != registerTaskList_.end(); )
    {
        if (zookeeper_->createZNode(it->first, it->second))
        {
            it = registerTaskList_.erase(it);
        }
        else
        {
            it ++;
        }
    }
}
