#include "NodeManager.h"

#include <sstream>

using namespace sf1r;

NodeManager::NodeManager()
: registered_(false), masterPort_(0), workerPort_(0)
{
}

NodeManager::~NodeManager()
{
    //zookeeper_->deleteZNode(nodePath_, true);
}

void NodeManager::initZooKeeper(const std::string& zkHosts, const int recvTimeout)
{
    zookeeper_.reset(new ZooKeeper(zkHosts, recvTimeout));
    zookeeper_->registerEventHandler(this);
}

void NodeManager::setCurrentNodeInfo(SF1NodeInfo& sf1NodeInfo)
{
    nodeInfo_ = sf1NodeInfo;
    nodePath_ = NodeUtil::getNodePath(nodeInfo_.mirrorId_, nodeInfo_.nodeId_);
}

void NodeManager::registerNode()
{
    if (zookeeper_->isConnected())
    {
        //std::cout<<"[NodeManager] connected to ZooKeeper Service."<<std::endl;

        ensureNodeParents(nodeInfo_.nodeId_, nodeInfo_.mirrorId_);

        zookeeper_->createZNode(nodePath_, nodeInfo_.localHost_);
        if (zookeeper_->isZNodeExists(nodePath_))
        {
            registered_ = true;
        }
    }
    else
    {
        std::cout<<"[NodeManager] connecting to ZooKeeper Service..."<<std::endl;
    }
}

void NodeManager::registerMaster(unsigned int port)
{
    masterPort_ = port;

    std::string path = nodePath_+"/Master";
    std::stringstream data;
    data << masterPort_;

    if (zookeeper_->isConnected())
    {
        zookeeper_->createZNode(path, data.str(), ZooKeeper::ZNODE_EPHEMERAL);
    }
}

void NodeManager::registerWorker(unsigned int port)
{
    workerPort_ = port;

    std::string path = nodePath_+"/Worker";
    std::stringstream data;
    data << workerPort_;

    if (zookeeper_->isConnected())
    {
        zookeeper_->createZNode(path, data.str(), ZooKeeper::ZNODE_EPHEMERAL);
    }
}

void NodeManager::process(ZooKeeperEvent& zkEvent)
{
    if (zookeeper_->isConnected())
    {
        //std::cout << "NodeManager::delayedProcess() "<<std::endl;
        delayedProcess();
    }
}

void NodeManager::ensureNodeParents(nodeid_t nodeId, mirrorid_t mirrorId)
{
    zookeeper_->createZNode(NodeUtil::getSF1RootPath());
    zookeeper_->createZNode(NodeUtil::getSF1TopologyPath());
    zookeeper_->createZNode(NodeUtil::getMirrorPath(mirrorId));
}

void NodeManager::delayedProcess()
{
    if (!registered_)
    {
        registerNode();

        // If parent node not registered, Master and Worker will also have not been registered.
        if (masterPort_ != 0)
            registerMaster(masterPort_);
        if (workerPort_ != 0)
            registerWorker(workerPort_);
    }
}
