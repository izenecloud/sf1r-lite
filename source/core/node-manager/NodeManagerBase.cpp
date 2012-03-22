#include "NodeManagerBase.h"
#include "SuperNodeManager.h"

#include <sstream>

using namespace sf1r;


NodeManagerBase::NodeManagerBase()
: nodeState_(NODE_STATE_INIT)
, masterStarted_(false)
, CLASSNAME("[NodeManagerBase]")
{
}

NodeManagerBase::~NodeManagerBase()
{
    stop();
}

void NodeManagerBase::init(const DistributedTopologyConfig& dsTopologyConfig)
{
    // set distributed topology configuration
    dsTopologyConfig_ = dsTopologyConfig;

    nodeInfo_.replicaId_ = dsTopologyConfig_.curSF1Node_.replicaId_;
    nodeInfo_.nodeId_ = dsTopologyConfig_.curSF1Node_.nodeId_;
    nodeInfo_.host_ = SuperNodeManager::get()->getLocalHostIP();
    nodeInfo_.baPort_ = SuperNodeManager::get()->getBaPort();

    zookeeper_ = ZooKeeperManager::get()->createClient(this);

    setZNodePaths(); // virtual
}

void NodeManagerBase::start()
{
    if (!dsTopologyConfig_.enabled_)
    {
        // not a distributed deployment
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

void NodeManagerBase::stop()
{
    if (masterStarted_)
    {
        stopMasterManager();
    }

    leaveCluster();
}

void NodeManagerBase::process(ZooKeeperEvent& zkEvent)
{
    std::cout<<CLASSNAME<< zkEvent.toString();

    if (zkEvent.type_ == ZOO_SESSION_EVENT && zkEvent.state_ == ZOO_CONNECTED_STATE)
    {
        if (nodeState_ == NODE_STATE_INIT)
        {

        }
        else if (nodeState_ == NODE_STATE_STARTING_WAIT_RETRY)
        {
            // retry start
            nodeState_ = NODE_STATE_STARTING;
            enterCluster();
        }
    }

    // ZOO_EXPIRED_SESSION_STATE
}

/// protected ////////////////////////////////////////////////////////////////////

void NodeManagerBase::initZkNameSpace()
{
    // Make sure zookeeper namaspace (znodes) is initialized properly
    zookeeper_->createZNode(clusterPath_);
    // topology
    zookeeper_->createZNode(topologyPath_);
    std::stringstream ss;
    ss << nodeInfo_.replicaId_;
    zookeeper_->createZNode(replicaPath_, ss.str());
}

void NodeManagerBase::enterCluster()
{
    boost::unique_lock<boost::mutex> lock(mutex_);

    if (nodeState_ == NODE_STATE_STARTED)
        return;

    // Ensure connecting to zookeeper
    if (!zookeeper_->isConnected())
    {
        zookeeper_->connect(true);

        if (!zookeeper_->isConnected())
        {
            // If still not connected, assume zookeeper service was stopped
            // and waiting for later connection after zookeeper recovered.
            nodeState_ = NODE_STATE_STARTING_WAIT_RETRY;
            std::cout<<CLASSNAME<<" waiting for ZooKeeper Service..."<<std::endl;
            return;
        }
    }

    // xxx Initialize zookeeper namespace for SF1
    initZkNameSpace();

    // Set zookeeper node data
    ZNode znode;
    znode.setValue(ZNode::KEY_HOST, SuperNodeManager::get()->getLocalHostIP());
    znode.setValue(ZNode::KEY_BA_PORT, SuperNodeManager::get()->getBaPort());
    znode.setValue(ZNode::KEY_DATA_PORT, SuperNodeManager::get()->getDataReceiverPort());
    znode.setValue(ZNode::KEY_MASTER_PORT, SuperNodeManager::get()->getMasterPort());
    if (dsTopologyConfig_.curSF1Node_.workerAgent_.enabled_)
    {
        znode.setValue(ZNode::KEY_WORKER_PORT, SuperNodeManager::get()->getWorkerPort());
        znode.setValue(ZNode::KEY_SHARD_ID, dsTopologyConfig_.curSF1Node_.workerAgent_.shardId_);
    }

    std::vector<std::string>& collectionList = dsTopologyConfig_.curSF1Node_.b5mServer_.collectionList_;
    std::string collections;
    for (std::vector<std::string>::iterator it = collectionList.begin();
            it != collectionList.end(); it++)
    {
        if (collections.empty())
            collections = *it;
        else
            collections += "," + *it;
    }
    znode.setValue(ZNode::KEY_COLLECTION, collections);

    // Register node to zookeeper
    std::string sznode = znode.serialize();
    if (!zookeeper_->createZNode(nodePath_, sznode, ZooKeeper::ZNODE_EPHEMERAL))
    {
        if (zookeeper_->getErrorCode() == ZooKeeper::ZERR_ZNODEEXISTS)
        {
            zookeeper_->setZNodeData(nodePath_, sznode);
            std::cout<<CLASSNAME<<" Conflict!! overwrote exsited node \""<<nodePath_<<"\"!"<<std::endl;
        }
        else
        {
            nodeState_ = NODE_STATE_STARTING_WAIT_RETRY;
            std::cout<<CLASSNAME<<" Failed to start ("<<zookeeper_->getErrorString()
                     <<"), waiting retry ..."<<std::endl;
            return;
        }
    }

    nodeState_ = NODE_STATE_STARTED;
    //std::cout<<CLASSNAME<<" node registered at \""<<nodePath_<<"\" "<<std::endl;
    std::cout<<CLASSNAME<<" joined cluster ["<<SuperNodeManager::get()->getClusterId()<<"] - "<<nodeInfo_.toString()<<std::endl;

    // Start Master manager
    if (dsTopologyConfig_.curSF1Node_.masterAgent_.enabled_)
    {
        startMasterManager(); // virtual
        masterStarted_ = true;
    }
}

void NodeManagerBase::leaveCluster()
{
    zookeeper_->deleteZNode(nodePath_, true);

    std::string replicaPath = replicaPath_;
    std::vector<std::string> childrenList;
    zookeeper_->getZNodeChildren(replicaPath, childrenList, ZooKeeper::NOT_WATCH, false);
    if (childrenList.size() <= 0)
    {
        zookeeper_->deleteZNode(replicaPath);
    }

    childrenList.clear();
    zookeeper_->getZNodeChildren(topologyPath_, childrenList, ZooKeeper::NOT_WATCH, false);
    if (childrenList.size() <= 0)
    {
        zookeeper_->deleteZNode(topologyPath_);
    }

    childrenList.clear();
    zookeeper_->getZNodeChildren(clusterPath_, childrenList, ZooKeeper::NOT_WATCH, false);
    if (childrenList.size() <= 0)
    {
        zookeeper_->deleteZNode(clusterPath_);
    }
}
