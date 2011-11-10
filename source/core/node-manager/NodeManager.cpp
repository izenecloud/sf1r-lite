#include "NodeManager.h"
#include "NodeData.h"

#include <node-manager/DistributedSynchroFactory.h>

#include <sstream>

using namespace sf1r;
using namespace zookeeper;

NodeManager::NodeManager()
: nodeState_(NODE_STATE_INIT)
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

    ///initZKNodes();
}

void NodeManager::start()
{
    if (!dsTopologyConfig_.enabled_)
    {
        // not invovled in distributed deployment
        return;
    }

    if (nodeState_ == NODE_STATE_INIT)
    {
        nodeState_ = NODE_STATE_STARTING;
        enterCluster();
    }
    else
    {
        // start once
        return;
    }
}

void NodeManager::stop()
{
    leaveCluster();
}

//void NodeManager::registerNode()
//{
//    registercalled_ = true;
//
//    if (zookeeper_->isConnected())
//    {
//        ensureNodeParents(nodeInfo_.nodeId_, nodeInfo_.replicaId_);
//        if (!zookeeper_->createZNode(nodePath_, nodeInfo_.localHost_))
//        {
//            if (zookeeper_->getErrorCode() == ZooKeeper::ZERR_ZNODEEXISTS)
//            {
//                zookeeper_->setZNodeData(nodePath_, nodeInfo_.localHost_);
//                std::cout<<"[NodeManager] warning: overwrote exsited node \""<<nodePath_<<"\"!"<<std::endl;
//            }
//        }
//
//        if (zookeeper_->isZNodeExists(nodePath_))
//        {
//            std::cout<<"[NodeManager] node registered at \""<<nodePath_<<"\" "<<nodeInfo_.localHost_<<std::endl;
//            registered_ = true;
//        }
//    }
//    else
//        std::cout<<"[NodeManager] waiting for ZooKeeper Service..."<<std::endl;
//}
//
//void NodeManager::registerMaster()
//{
//    masterPort_ = dsTopologyConfig_.curSF1Node_.masterAgent_.port_;
//
//    std::string path = nodePath_+"/Master";
//    std::stringstream data;
//    data << masterPort_;
//
//    if (registered_)
//    {
//        zookeeper_->createZNode(path, data.str(), ZooKeeper::ZNODE_EPHEMERAL);
//    }
//}
//
//void NodeManager::registerWorker()
//{
//    workerPort_ = dsTopologyConfig_.curSF1Node_.workerAgent_.port_;
//
//    std::string path = nodePath_+"/Worker";
//    std::stringstream data;
//    data << workerPort_;
//
//    if (registered_)
//    {
//        zookeeper_->createZNode(path, data.str(), ZooKeeper::ZNODE_EPHEMERAL);
//    }
//}

void NodeManager::process(ZooKeeperEvent& zkEvent)
{
    std::cout<<"[NodeManager] "<< zkEvent.toString();

    if (zkEvent.type_ == ZOO_SESSION_EVENT && zkEvent.state_ == ZOO_CONNECTED_STATE)
    {
        if (nodeState_ == NODE_STATE_STARTING_WAIT_RETRY)
        {
            // retry start
            nodeState_ = NODE_STATE_STARTING;
            enterCluster();
        }

//        if (!inited_)
//            initZKNodes();
//
//        if (registercalled_ && !registered_)
//        {
//            retryRegister();
//        }
    }
}

/// private ////////////////////////////////////////////////////////////////////

void NodeManager::initZooKeeper(const std::string& zkHosts, const int recvTimeout)
{
    zookeeper_.reset(new ZooKeeper(zkHosts, recvTimeout));
    zookeeper_->registerEventHandler(this);
}

void NodeManager::initZkNameSpace()
{
    // Make sure zookeeper namaspace (znodes) is initialized properly
    // for all distributed coordination tasks.

    zookeeper_->createZNode(NodeDef::getSF1RootPath());
    // topology
    zookeeper_->createZNode(NodeDef::getSF1TopologyPath());
    zookeeper_->createZNode(NodeDef::getReplicaPath(dsTopologyConfig_.curSF1Node_.replicaId_));
    // synchro, todo
    zookeeper_->createZNode(NodeDef::getSynchroPath());
    DistributedSynchroFactory::initZKNodes(zookeeper_);
}

void NodeManager::enterCluster()
{
    // Not strictly synchronized here, it's not fatal.
    std::cout<<"[NodeManager] staring ..."<<std::endl;

    // Ensure connecting to zookeeper
    if (!zookeeper_->isConnected())
    {
        zookeeper_->connect(true);

        if (!zookeeper_->isConnected())
        {
            // If still not connected, assume zookeeper service was stopped
            // and waiting for later process after zookeeper recovered.
            nodeState_ = NODE_STATE_STARTING_WAIT_RETRY;
            std::cout<<"[NodeManager] waiting for ZooKeeper Service..."<<std::endl;
            return;
        }
    }

    // Initialize zookeeper namespace for SF1 !!
    initZkNameSpace();

    // Set node info
    NodeData ndata;
    ndata.setValue(NDATA_KEY_HOST, dsTopologyConfig_.curSF1Node_.host_);
    ndata.setValue(NDATA_KEY_BA_PORT, dsTopologyConfig_.curSF1Node_.baPort_);
    if (dsTopologyConfig_.curSF1Node_.masterAgent_.enabled_)
    {
        ndata.setValue(NDATA_KEY_MASTER_PORT, dsTopologyConfig_.curSF1Node_.masterAgent_.port_);

        // start master here?
    }
    if (dsTopologyConfig_.curSF1Node_.workerAgent_.enabled_)
    {
        ndata.setValue(NDATA_KEY_WORKER_PORT, dsTopologyConfig_.curSF1Node_.workerAgent_.port_);
        ndata.setValue(NDATA_KEY_SHARD_ID, dsTopologyConfig_.curSF1Node_.workerAgent_.port_);
    }

    // Register node to zookeeper
    std::string sndata = ndata.serialize();
    if (!zookeeper_->createZNode(nodePath_, sndata, ZooKeeper::ZNODE_EPHEMERAL))
    {
        if (zookeeper_->getErrorCode() == ZooKeeper::ZERR_ZNODEEXISTS)
        {
            zookeeper_->setZNodeData(nodePath_, sndata);
            std::cout<<"[NodeManager] warning: overwrote exsited node \""<<nodePath_<<"\"!"<<std::endl;
        }
        else
        {
            nodeState_ = NODE_STATE_STARTING_WAIT_RETRY;
            std::cout<<"[NodeManager] failed to start (err:"<<zookeeper_->getErrorCode()
                     <<"), waiting retry ..."<<std::endl;
            return;
        }
    }

    std::cout<<"[NodeManager] node registered at \""<<nodePath_<<"\" "<<std::endl;
    nodeState_ = NODE_STATE_STARTED;
}

void NodeManager::leaveCluster()
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
        zookeeper_->deleteZNode(NodeDef::getSF1TopologyPath());
    }

    childrenList.clear();
    zookeeper_->getZNodeChildren(NodeDef::getSF1RootPath(), childrenList, ZooKeeper::NOT_WATCH, false);
    if (childrenList.size() <= 0)
    {
        zookeeper_->deleteZNode(NodeDef::getSF1RootPath());
    }
}

//void NodeManager::ensureNodeParents(nodeid_t nodeId, replicaid_t replicaId)
//{
//    zookeeper_->createZNode(NodeDef::getSF1RootPath());
//    zookeeper_->createZNode(NodeDef::getSF1TopologyPath());
//    zookeeper_->createZNode(NodeDef::getReplicaPath(replicaId));
//}
//
//void NodeManager::initZKNodes()
//{
//    if (zookeeper_->isConnected())
//    {
//        // SF1 maybe exited abnormally last time
//        zookeeper_->createZNode(NodeDef::getSF1RootPath());
//        // topology, xxx
//        zookeeper_->createZNode(NodeDef::getSF1TopologyPath());
//        // synchro
//        zookeeper_->createZNode(NodeDef::getSynchroPath());
//        DistributedSynchroFactory::initZKNodes(zookeeper_);
//        inited_ = true;
//    }
//}
//
//void NodeManager::retryRegister()
//{
//    //std::cout<<"NodeManager::retryRegister"<<std::endl;
//    registerNode();
//
//    if (masterPort_ != 0)
//        registerMaster();
//    if (workerPort_ != 0)
//        registerWorker();
//}
