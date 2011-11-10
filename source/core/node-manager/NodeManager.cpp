#include "NodeManager.h"

#include <node-manager/DistributedSynchroFactory.h>

#include <sstream>

using namespace sf1r;

NodeManager::NodeManager()
: inited_(false), registercalled_(false), registered_(false), masterPort_(0), workerPort_(0)
{
}

NodeManager::~NodeManager()
{
}

void NodeManager::initWithConfig(
        const DistributedTopologyConfig& dsTopologyConfig,
        const DistributedUtilConfig& dsUtilConfig)
{
    // set distributed configurations
    dsTopologyConfig_ = dsTopologyConfig;
    dsUtilConfig_ = dsUtilConfig;

    // initialization
    NodeDef::setClusterIdNodeName(dsTopologyConfig_.clusterId_);

    initZooKeeper(dsUtilConfig_.zkConfig_.zkHosts_, dsUtilConfig_.zkConfig_.zkRecvTimeout_);

    nodeInfo_.replicaId_ = dsTopologyConfig_.curSF1Node_.replicaId_;
    nodeInfo_.nodeId_ = dsTopologyConfig_.curSF1Node_.nodeId_;
    nodeInfo_.localHost_ = dsTopologyConfig_.curSF1Node_.host_;
    nodeInfo_.baPort_ = dsTopologyConfig_.curSF1Node_.baPort_;

    nodePath_ = NodeDef::getNodePath(nodeInfo_.replicaId_, nodeInfo_.nodeId_);

    initZKNodes();
}

void NodeManager::initZooKeeper(const std::string& zkHosts, const int recvTimeout)
{
    zookeeper_.reset(new ZooKeeper(zkHosts, recvTimeout));
    zookeeper_->registerEventHandler(this);
}

void NodeManager::registerNode()
{
    registercalled_ = true;

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
    else
        std::cout<<"[NodeManager] waiting for ZooKeeper Service..."<<std::endl;
}

void NodeManager::registerMaster()
{
    masterPort_ = dsTopologyConfig_.curSF1Node_.masterAgent_.port_;

    std::string path = nodePath_+"/Master";
    std::stringstream data;
    data << masterPort_;

    if (registered_)
    {
        zookeeper_->createZNode(path, data.str(), ZooKeeper::ZNODE_EPHEMERAL);
    }
}

void NodeManager::registerWorker()
{
    workerPort_ = dsTopologyConfig_.curSF1Node_.workerAgent_.port_;

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
    std::cout<<"[NodeManager] "<< zkEvent.toString();

    if (zkEvent.type_ == ZOO_SESSION_EVENT && zkEvent.state_ == ZOO_CONNECTED_STATE)
    {
        if (!inited_)
            initZKNodes();

        if (registercalled_ && !registered_)
        {
            retryRegister();
        }
    }
}

void NodeManager::ensureNodeParents(nodeid_t nodeId, replicaid_t replicaId)
{
    zookeeper_->createZNode(NodeDef::getSF1RootPath());
    zookeeper_->createZNode(NodeDef::getSF1TopologyPath());
    zookeeper_->createZNode(NodeDef::getReplicaPath(replicaId));
}

void NodeManager::initZKNodes()
{
    if (zookeeper_->isConnected())
    {
        // SF1 maybe exited abnormally last time
        zookeeper_->createZNode(NodeDef::getSF1RootPath());
        // topology, xxx
        zookeeper_->createZNode(NodeDef::getSF1TopologyPath());
        // synchro
        zookeeper_->createZNode(NodeDef::getSynchroPath());
        DistributedSynchroFactory::initZKNodes(zookeeper_);
        inited_ = true;
    }
}

void NodeManager::retryRegister()
{
    //std::cout<<"NodeManager::retryRegister"<<std::endl;
    registerNode();

    if (masterPort_ != 0)
        registerMaster();
    if (workerPort_ != 0)
        registerWorker();
}
