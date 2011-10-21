#include "NodeManager.h"

#include <sstream>

using namespace sf1r;

NodeManager::NodeManager()
: registered_(false), masterPort_(0), workerPort_(0)
{
}

NodeManager::~NodeManager()
{
}

void NodeManager::initZooKeeper(const std::string& zkHosts, const int recvTimeout)
{
    zookeeper_.reset(new ZooKeeper(zkHosts, recvTimeout));
    zookeeper_->registerEventHandler(this);
}

void NodeManager::setCurrentNodeInfo(SF1NodeInfo& sf1NodeInfo)
{
    nodeInfo_ = sf1NodeInfo;
    nodePath_ = NodeDef::getNodePath(nodeInfo_.replicaId_, nodeInfo_.nodeId_);
}

void NodeManager::registerNode()
{
    if (zookeeper_->isConnected())
    {
        ensureNodeParents(nodeInfo_.nodeId_, nodeInfo_.replicaId_);
        if (!zookeeper_->createZNode(nodePath_, nodeInfo_.localHost_))
        {
            if (zookeeper_->getErrorCode() == ZooKeeper::ZERR_ZNODEEXISTS)
            {
                zookeeper_->setZNodeData(nodePath_, nodeInfo_.localHost_);
                std::cout<<"[NodeManager] warning: overwrote exsited node \""<<nodePath_<<"\"!"<<std::endl;
            }
        }

        if (zookeeper_->isZNodeExists(nodePath_))
        {
            std::cout<<"[NodeManager] node registered at \""<<nodePath_<<"\" "<<nodeInfo_.localHost_<<std::endl;
            registered_ = true;
        }
    }

    std::cout<<"[NodeManager] waiting for ZooKeeper Service..."<<std::endl;
}

void NodeManager::registerMaster(unsigned int port)
{
    masterPort_ = port;

    std::string path = nodePath_+"/Master";
    std::stringstream data;
    data << masterPort_;

    if (registered_)
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

    if (registered_)
    {
        zookeeper_->createZNode(path, data.str(), ZooKeeper::ZNODE_EPHEMERAL);
    }
}

void NodeManager::deregisterNode()
{
    zookeeper_->deleteZNode(nodePath_, true);

    std::string replicaPath = NodeDef::getReplicaPath(nodeInfo_.replicaId_);
    std::vector<std::string> childrenList;
    zookeeper_->getZNodeChildren(replicaPath, childrenList, ZooKeeper::NOT_WATCH, false);
    if (childrenList.size() <= 0)
    {
        zookeeper_->deleteZNode(replicaPath);
    }

    childrenList.clear();
    zookeeper_->getZNodeChildren(NodeDef::getSF1TopologyPath(), childrenList, ZooKeeper::NOT_WATCH, false);
    if (childrenList.size() <= 0)
    {
        // xxx ?
        zookeeper_->deleteZNode(NodeDef::getSF1RootPath(), true);
    }
}

void NodeManager::process(ZooKeeperEvent& zkEvent)
{
    ///std::cout<<"NodeManager::process "<< zkEvent.toString();

    if (!registered_)
    {
        retryRegister();
    }
}

void NodeManager::ensureNodeParents(nodeid_t nodeId, replicaid_t replicaId)
{
    zookeeper_->createZNode(NodeDef::getSF1RootPath());
    zookeeper_->createZNode(NodeDef::getSF1TopologyPath());
    zookeeper_->createZNode(NodeDef::getReplicaPath(replicaId));
}

void NodeManager::retryRegister()
{
    //std::cout<<"NodeManager::retryRegister"<<std::endl;
    registerNode();

    if (masterPort_ != 0)
        registerMaster(masterPort_);
    if (workerPort_ != 0)
        registerWorker(workerPort_);
}
