#include "NodeManagerBase.h"
#include "SuperNodeManager.h"

#include <sstream>

#include <boost/lexical_cast.hpp>


namespace sf1r
{

NodeManagerBase::NodeManagerBase()
    : isDistributionEnabled_(false)
    , nodeState_(NODE_STATE_INIT)
    , masterStarted_(false)
    , CLASSNAME("[NodeManagerBase]")
{
}

NodeManagerBase::~NodeManagerBase()
{
    stop();
}

void NodeManagerBase::init(const DistributedTopologyConfig& distributedTopologyConfig)
{
    isDistributionEnabled_ = distributedTopologyConfig.enabled_;
    sf1rTopology_ = distributedTopologyConfig.sf1rTopology_;

    zookeeper_ = ZooKeeperManager::get()->createClient(this);
    setZNodePaths();
}

void NodeManagerBase::start()
{
    if (!isDistributionEnabled_)
    {
        return;
    }

    if (nodeState_ == NODE_STATE_INIT)
    {
        nodeState_ = NODE_STATE_STARTING;
        enterCluster();
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
    LOG (INFO) << CLASSNAME << " " << zkEvent.toString();

    if (zkEvent.type_ == ZOO_SESSION_EVENT && zkEvent.state_ == ZOO_CONNECTED_STATE)
    {
        if (nodeState_ == NODE_STATE_STARTING_WAIT_RETRY)
        {
            // retry start
            nodeState_ = NODE_STATE_STARTING;
            enterCluster();
        }
    }

    // ZOO_EXPIRED_SESSION_STATE
}

/// protected ////////////////////////////////////////////////////////////////////

void NodeManagerBase::tryInitZkNameSpace()
{
    zookeeper_->createZNode(clusterPath_);
    zookeeper_->createZNode(topologyPath_);
    std::stringstream ss;
    ss << sf1rTopology_.curNode_.replicaId_;
    zookeeper_->createZNode(replicaPath_, ss.str());
}

bool NodeManagerBase::checkZooKeeperService()
{
    if (!zookeeper_->isConnected())
    {
        zookeeper_->connect(true);

        if (!zookeeper_->isConnected())
        {
            // If still not connected, assume zookeeper service was stopped
            // and waiting for later connection after zookeeper recovered.
            nodeState_ = NODE_STATE_STARTING_WAIT_RETRY;
            return false;
        }
    }

    return true;
}

void NodeManagerBase::setSf1rNodeData(ZNode& znode)
{
    znode.setValue(ZNode::KEY_USERNAME, sf1rTopology_.curNode_.userName_);
    znode.setValue(ZNode::KEY_HOST, sf1rTopology_.curNode_.host_);
    znode.setValue(ZNode::KEY_BA_PORT, sf1rTopology_.curNode_.baPort_);
    znode.setValue(ZNode::KEY_DATA_PORT, sf1rTopology_.curNode_.dataPort_);

    if (sf1rTopology_.curNode_.worker_.isEnabled_)
    {
        znode.setValue(ZNode::KEY_WORKER_PORT, sf1rTopology_.curNode_.worker_.port_);
        znode.setValue(ZNode::KEY_SHARD_ID, sf1rTopology_.curNode_.worker_.shardId_);
    }

    if (sf1rTopology_.curNode_.master_.isEnabled_)
    {
        znode.setValue(ZNode::KEY_MASTER_PORT, sf1rTopology_.curNode_.master_.port_);
    }
    //if (sf1rTopology_.curNode_.master_.isEnabled_)
    {
        std::string collections;
        std::vector<MasterCollection>& collectionList = sf1rTopology_.curNode_.master_.collectionList_;
        for (std::vector<MasterCollection>::iterator it = collectionList.begin();
                it != collectionList.end(); it++)
        {
            if (collections.empty())
                collections = (*it).name_;
            else
                collections += "," + (*it).name_;
        }
        znode.setValue(ZNode::KEY_COLLECTION, collections);
    }
}

void NodeManagerBase::enterCluster()
{
    boost::unique_lock<boost::mutex> lock(mutex_);
    if (nodeState_ == NODE_STATE_STARTED)
    {
        return;
    }

    if (!checkZooKeeperService())
    {
        // process will be resumed after zookeeper recovered
        LOG (WARNING) << CLASSNAME << " waiting for ZooKeeper Service ...";
        return;
    }

    // ensure base paths
    tryInitZkNameSpace();

    // register current sf1r node to ZooKeeper
    ZNode znode;
    setSf1rNodeData(znode);
    if (!zookeeper_->createZNode(nodePath_, znode.serialize(), ZooKeeper::ZNODE_EPHEMERAL))
    {
        if (zookeeper_->getErrorCode() == ZooKeeper::ZERR_ZNODEEXISTS)
        {
            std::string data;
            zookeeper_->getZNodeData(nodePath_, data);

            ZNode znode;
            znode.loadKvString(data);

            std::stringstream ss;
            ss << CLASSNAME << " Conflicted with existed node: "
               << "[replica "<< sf1rTopology_.curNode_.replicaId_
               << ", node " << sf1rTopology_.curNode_.nodeId_ << "]@"
               << znode.getStrValue(ZNode::KEY_HOST);

            throw std::runtime_error(ss.str());
        }
        else
        {
            nodeState_ = NODE_STATE_STARTING_WAIT_RETRY;
            LOG (ERROR) << CLASSNAME << " Failed to start (" << zookeeper_->getErrorString()
                        << "), waiting for retry ..." << std::endl;
            return;
        }
    }

    nodeState_ = NODE_STATE_STARTED;

    Sf1rNode& curNode = sf1rTopology_.curNode_;
    LOG (INFO) << CLASSNAME
               << " started, cluster[" << sf1rTopology_.clusterId_
               << "] replica[" << curNode.replicaId_
               << "] node[" << curNode.nodeId_
               << "]{"
               << (curNode.worker_.isEnabled_ ? (std::string("worker ") + boost::lexical_cast<std::string>(curNode.worker_.shardId_) + " ") : "")
               << curNode.userName_ << "@" << curNode.host_ << "}";


    // Start Master manager
    if (sf1rTopology_.curNode_.master_.isEnabled_)
    {
        startMasterManager();
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

}
