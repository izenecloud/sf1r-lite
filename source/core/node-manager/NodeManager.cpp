#include "NodeManager.h"

#include <node-manager/synchro/SynchroFactory.h>

#include <sstream>

using namespace sf1r;


NodeManager::NodeManager()
: isInitBeforeStartDone_(false)
, nodeState_(NODE_STATE_INIT)
, masterStarted_(false)
, CLASSNAME("[NodeManager]")
{
}

NodeManager::~NodeManager()
{
    stop();
}

void NodeManager::init(
        const DistributedTopologyConfig& dsTopologyConfig,
        const DistributedUtilConfig& dsUtilConfig)
{
    // Initializations which should be done before collections started.

    // set distributed configurations
    dsTopologyConfig_ = dsTopologyConfig;
    dsUtilConfig_ = dsUtilConfig;

    // initialization
    NodeDef::setClusterIdNodeName(dsTopologyConfig_.clusterId_);

    zookeeper_ = ZooKeeperManager::get()->createClient(this);

    nodeInfo_.replicaId_ = dsTopologyConfig_.curSF1Node_.replicaId_;
    nodeInfo_.nodeId_ = dsTopologyConfig_.curSF1Node_.nodeId_;
    nodeInfo_.host_ = dsTopologyConfig_.curSF1Node_.host_;
    nodeInfo_.baPort_ = dsTopologyConfig_.curSF1Node_.baPort_;

    nodePath_ = NodeDef::getNodePath(nodeInfo_.replicaId_, nodeInfo_.nodeId_);

    // !! Initializations needed to be done before start collections (run)
    initBeforeStart();
}

void NodeManager::start()
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

void NodeManager::stop()
{
    if (masterStarted_)
    {
        stopMasterManager();
    }

    leaveCluster();
}

void NodeManager::process(ZooKeeperEvent& zkEvent)
{
    std::cout<<CLASSNAME<< zkEvent.toString();

    if (!isInitBeforeStartDone_)
    {
        initBeforeStart();
    }

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

void NodeManager::initBeforeStart()
{
    if (zookeeper_->isConnected())
    {
        // xxx synchro
        SynchroFactory::initSynchroNode(zookeeper_);

        isInitBeforeStartDone_ = true;
    }
}

void NodeManager::initZkNameSpace()
{
    // Make sure zookeeper namaspace (znodes) is initialized properly

    zookeeper_->createZNode(NodeDef::getSF1RootPath());
    // topology
    zookeeper_->createZNode(NodeDef::getSF1TopologyPath());
    std::stringstream ss;
    ss << dsTopologyConfig_.curSF1Node_.replicaId_;
    zookeeper_->createZNode(NodeDef::getReplicaPath(dsTopologyConfig_.curSF1Node_.replicaId_), ss.str());
}

void NodeManager::enterCluster()
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

    // Initialize zookeeper namespace for SF1 !!
    initZkNameSpace();

    // Set node info
    NodeData ndata;
    ndata.setValue(NodeData::NDATA_KEY_HOST, dsTopologyConfig_.curSF1Node_.host_);
    ndata.setValue(NodeData::NDATA_KEY_BA_PORT, dsTopologyConfig_.curSF1Node_.baPort_);
    ndata.setValue(NodeData::NDATA_KEY_DATA_PORT, dsTopologyConfig_.curSF1Node_.dataPort_);
    if (dsTopologyConfig_.curSF1Node_.masterAgent_.enabled_)
    {
        ndata.setValue(NodeData::NDATA_KEY_MASTER_PORT, dsTopologyConfig_.curSF1Node_.masterAgent_.port_);
    }
    if (dsTopologyConfig_.curSF1Node_.workerAgent_.enabled_)
    {
        ndata.setValue(NodeData::NDATA_KEY_WORKER_PORT, dsTopologyConfig_.curSF1Node_.workerAgent_.port_);
        ndata.setValue(NodeData::NDATA_KEY_SHARD_ID, dsTopologyConfig_.curSF1Node_.workerAgent_.shardId_);
    }

    // Register node to zookeeper
    std::string sndata = ndata.serialize();
    if (!zookeeper_->createZNode(nodePath_, sndata, ZooKeeper::ZNODE_EPHEMERAL))
    {
        if (zookeeper_->getErrorCode() == ZooKeeper::ZERR_ZNODEEXISTS)
        {
            zookeeper_->setZNodeData(nodePath_, sndata);
            std::cout<<CLASSNAME<<" Conflict!! overwrote exsited node \""<<nodePath_<<"\"!"<<std::endl;
        }
        else
        {
            nodeState_ = NODE_STATE_STARTING_WAIT_RETRY;
            std::cout<<CLASSNAME<<" failed to start (err:"<<zookeeper_->getErrorCode()
                     <<"), waiting retry ..."<<std::endl;
            return;
        }
    }

    nodeState_ = NODE_STATE_STARTED;
    //std::cout<<CLASSNAME<<" node registered at \""<<nodePath_<<"\" "<<std::endl;
    std::cout<<CLASSNAME<<" joined cluster ["<<dsTopologyConfig_.clusterId_<<"] - "<<nodeInfo_.toString()<<std::endl;

    // Start Master manager
    if (dsTopologyConfig_.curSF1Node_.masterAgent_.enabled_)
    {
        startMasterManager(); // virtual
        masterStarted_ = true;
    }
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
