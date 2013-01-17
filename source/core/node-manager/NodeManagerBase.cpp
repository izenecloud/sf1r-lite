#include "NodeManagerBase.h"
#include "SuperNodeManager.h"
#include "SuperMasterManager.h"
#include "RequestLog.h"

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
    setMasterDistributeState(isDistributionEnabled_);
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
        SuperMasterManager::get()->stop();
    }

    leaveCluster();
    nodeState_ = NODE_STATE_INIT;
}

void NodeManagerBase::process(ZooKeeperEvent& zkEvent)
{
    LOG (INFO) << CLASSNAME << " worker node event: " << zkEvent.toString();

    if (zkEvent.type_ == ZOO_SESSION_EVENT && zkEvent.state_ == ZOO_CONNECTED_STATE)
    {
        if (nodeState_ == NODE_STATE_STARTING_WAIT_RETRY)
        {
            // retry start
            nodeState_ = NODE_STATE_STARTING;
            enterCluster();
        }
    }

    if (zkEvent.type_ == ZOO_SESSION_EVENT && 
        zkEvent.state_ == ZOO_EXPIRED_SESSION_STATE)
    {
        // closed by zookeeper because of session expired
        LOG(WARNING) << "worker node disconnected by zookeeper, state: " << zookeeper_->getStateString();
        LOG(WARNING) << "try reconnect : " << sf1rTopology_.curNode_.toString();
        zookeeper_->disconnect();
        nodeState_ = NODE_STATE_STARTING;
        enterCluster(false);
        LOG (WARNING) << " restarted in NodeManagerBase for ZooKeeper Service finished";
    }
    if (zkEvent.type_ == ZOO_CHILD_EVENT)
    {
        detectMasters();
    }
}

/// protected ////////////////////////////////////////////////////////////////////

void NodeManagerBase::tryInitZkNameSpace()
{
    zookeeper_->createZNode(clusterPath_);
    zookeeper_->createZNode(topologyPath_);
    std::stringstream ss;
    ss << sf1rTopology_.curNode_.replicaId_;
    zookeeper_->createZNode(replicaPath_, ss.str());
    zookeeper_->createZNode(primaryBasePath_);
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
    znode.setValue(ZNode::KEY_REPLICA_ID, sf1rTopology_.curNode_.replicaId_);
    znode.setValue(ZNode::KEY_NODE_STATE, (uint32_t)nodeState_);
    znode.setValue(ZNode::KEY_SELF_REG_PRIMARY_PATH, self_primary_path_);

    if (sf1rTopology_.curNode_.worker_.isEnabled_)
    {
        znode.setValue(ZNode::KEY_WORKER_PORT, sf1rTopology_.curNode_.worker_.port_);
    }

    if (sf1rTopology_.curNode_.master_.isEnabled_)
    {
        znode.setValue(ZNode::KEY_MASTER_PORT, sf1rTopology_.curNode_.master_.port_);
        znode.setValue(ZNode::KEY_MASTER_NAME, sf1rTopology_.curNode_.master_.name_);
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

void NodeManagerBase::updateCurrentPrimary()
{
    std::vector<std::string> primaryList;
    zookeeper_->getZNodeChildren(primaryNodeParentPath_, primaryList, ZooKeeper::NOT_WATCH);
    if (primaryList.empty())
    {
        LOG(ERROR) << "primary is empty";
        curr_primary_path_.clear();
        return;
    }
    curr_primary_path_ = primaryList[0];
    LOG(INFO) << "current primary is : " << curr_primary_path_;
    getPrimaryState();
}

void NodeManagerBase::unregisterPrimary()
{
    if (self_primary_path_.empty())
    {
        return;
    }
    zookeeper_->deleteZNode(self_primary_path_);
    self_primary_path_.clear();
    ZNode znode;
    setSf1rNodeData(znode);
    zookeeper_->setZNodeData(nodePath_, znode.serialize());
}

void NodeManagerBase::registerPrimary(ZNode& znode)
{
    if (!zookeeper_->isZNodeExists(primaryNodeParentPath_))
    {
        zookeeper_->createZNode(primaryNodeParentPath_);
    }
    if (!self_primary_path_.empty() && zookeeper_->isZNodeExists(self_primary_path_))
    {
        LOG(WARNING) << "the node has register already, reregister will leaving duplicate primary node. old register path: " << self_primary_path_;
    }
    if (zookeeper_->createZNode(primaryNodePath_, znode.serialize(), ZooKeeper::ZNODE_EPHEMERAL_SEQUENCE))
    {
        self_primary_path_ = zookeeper_->getLastCreatedNodePath();
        LOG(INFO) << "current self node primary path is : " << self_primary_path_;
        znode.setValue(ZNode::KEY_SELF_REG_PRIMARY_PATH, self_primary_path_);
        zookeeper_->setZNodeData(nodePath_, znode.serialize());
        updateCurrentPrimary();
    }
    else
    {
        LOG(ERROR) << "register primary failed. " << sf1rTopology_.curNode_.toString();
    }
}

void NodeManagerBase::enterCluster(bool start_master)
{
    {
        boost::unique_lock<boost::mutex> lock(mutex_);
        if (nodeState_ == NODE_STATE_STARTED)
        {
            return;
        }

        if (nodeState_ == NODE_STATE_RECOVER_RUNNING ||
            nodeState_ == NODE_STATE_RECOVER_WAIT_PRIMARY)
        {
            LOG(WARNING) << "try to reenter cluster while node is recovering!" << sf1rTopology_.curNode_.toString();
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
                ss << CLASSNAME << " conflict: node existed "
                    << "[replica"<< sf1rTopology_.curNode_.replicaId_
                    << ", node" << sf1rTopology_.curNode_.nodeId_ << "]@"
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

        nodeState_ = NODE_STATE_RECOVER_RUNNING;
        updateCurrentPrimary();
    }
    LOG(INFO) << "begin recovering callback : " << self_primary_path_;
    if (cb_on_recovering_)
    {
        cb_on_recovering_();
    }

    boost::unique_lock<boost::mutex> lock(mutex_);
    // first register a sequence primary node, so that 
    // we can store this path to current node. the other
    // node can tell which primary path current node belong to.
    updateCurrentPrimary();
    if (curr_primary_path_.empty())
    {
        LOG(INFO) << "no primary worker currently.";
        LOG(INFO) << "I am starting as primary worker.";
    }
    else
    {
        nodeState_ = NODE_STATE_RECOVER_WAIT_PRIMARY;
        LOG(INFO) << "recover done and wait to sync primary after primary is idle." << self_primary_path_;
        if (getPrimaryState() == NODE_STATE_ELECTING)
        {
            LOG(INFO) << "primary changed while I am recovering, sync to new primary.";
            if (cb_on_recover_wait_primary_)
                cb_on_recover_wait_primary_();
        }
        else
        {
            ZNode znode;
            setSf1rNodeData(znode);
            registerPrimary(znode);
            updateNodeState();
            return;
        }
    }
    nodeState_ = NODE_STATE_STARTED;
    enterClusterAfterRecovery(start_master);
}

void NodeManagerBase::enterClusterAfterRecovery(bool start_master)
{
    if (self_primary_path_.empty())
    {
        ZNode znode;
        setSf1rNodeData(znode);
        registerPrimary(znode);
        if (curr_primary_path_ == self_primary_path_)
        {
            LOG(INFO) << "I enter as primary success." << self_primary_path_;
            nodeState_ = NODE_STATE_ELECTING;
            checkSecondaryElecting();
        }
        else
        {
            LOG(INFO) << "enter as primary fail, maybe another node is entering at the same time.";
            nodeState_ = NODE_STATE_RECOVER_WAIT_PRIMARY;
            updateNodeState();
            LOG(INFO) << "begin wait new entered primary and try re-enter when sync to new primary.";
            return;
        }
    }

    LOG(INFO) << "recovery finished. Begin enter cluster after recovery";
    updateNodeState();
    updateCurrentPrimary();

    Sf1rNode& curNode = sf1rTopology_.curNode_;
    LOG (INFO) << CLASSNAME
               << " started, cluster[" << sf1rTopology_.clusterId_
               << "] replica[" << curNode.replicaId_
               << "] node[" << curNode.nodeId_
               << "]{"
               << (curNode.worker_.isEnabled_ ?
                       (std::string("worker") + boost::lexical_cast<std::string>(curNode.nodeId_) + " ") : "")
               << curNode.userName_ << "@" << curNode.host_ << "}";


    if(start_master)
    {
        // Start Master manager
        if (sf1rTopology_.curNode_.master_.isEnabled_)
        {
            startMasterManager();
            SuperMasterManager::get()->init(sf1rTopology_);
            SuperMasterManager::get()->start();
            masterStarted_ = true;
        }
    }

    if (sf1rTopology_.curNode_.worker_.isEnabled_)
    {
        detectMasters();
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

bool NodeManagerBase::isPrimary()
{
    boost::unique_lock<boost::mutex> lock(mutex_);
    return isPrimaryWithoutLock();
}

bool NodeManagerBase::isPrimaryWithoutLock() const
{
    if (!zookeeper_ || !zookeeper_->isConnected())
        return false;
    if (nodeState_ == NODE_STATE_RECOVER_RUNNING || self_primary_path_.empty())
        return false;
    return curr_primary_path_ == self_primary_path_;
}

NodeManagerBase::NodeStateType NodeManagerBase::getNodeState(const std::string& nodepath)
{
    std::string data;
    if(!zookeeper_->getZNodeData(nodepath, data, ZooKeeper::WATCH))
    {
        LOG(INFO) << "get node data failed while get node state: " << nodepath;
        return NODE_STATE_UNKNOWN;
    }
    ZNode node;
    node.loadKvString(data);
    return (NodeStateType)node.getUInt32Value(ZNode::KEY_NODE_STATE);
}

NodeManagerBase::NodeStateType NodeManagerBase::getPrimaryState()
{
    NodeStateType state = getNodeState(curr_primary_path_);
    LOG(INFO) << "primary state is : " << state;
    return state;
}

void NodeManagerBase::beginReqProcess()
{
    setNodeState(NODE_STATE_PROCESSING_REQ_RUNNING);
}

void NodeManagerBase::abortRequest()
{
    // notify abort and wait other aborting.
    if (isPrimary())
    {
        LOG(INFO) << "primary abort the request.";
        setNodeState(NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_ABORT);
        checkSecondaryReqAbort();
    }
    else
    {
        LOG(INFO) << "replica abort the request.";
        setNodeState(NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY_ABORT);
    }

}

void NodeManagerBase::finishLocalReqProcess(int type, const std::string& packed_reqdata)
{
    if (isPrimary())
    {
        LOG(INFO) << "send request to other replicas from primary.";
        // write request data to node to notify replica.
        nodeState_ = NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_FINISH_PROCESS;
        ZNode znode;
        setSf1rNodeData(znode);
        znode.setValue(ZNode::KEY_PRIMARY_WORKER_REQ_DATA, packed_reqdata);
        znode.setValue(ZNode::KEY_REQ_TYPE, (uint32_t)type);
        zookeeper_->setZNodeData(self_primary_path_, znode.serialize());
    }
    else
    {
        LOG(INFO) << "replica finished local and begin waiting from primary.";
        setNodeState(NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY);
    }
}

void NodeManagerBase::onNodeDeleted(const std::string& path)
{
    boost::unique_lock<boost::mutex> lock(mutex_);
    if (path == nodePath_ || path == self_primary_path_)
    {
        LOG(INFO) << "myself node was deleted : " << path;
        return;
    }
    else if (path.find(primaryNodeParentPath_) == std::string::npos)
    {
        LOG(INFO) << "node was deleted, but I did not care : " << path;
        return;
    }
    else if (path == curr_primary_path_)
    {
        LOG(WARNING) << "primary node was deleted : " << path;
        updateCurrentPrimary();
        if (nodeState_ == NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY ||
            nodeState_ == NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY_ABORT)
        {
            LOG(INFO) << "stop waiting primary for current processing request.";
            if (cb_on_abort_request_)
                cb_on_abort_request_();
            nodeState_ = NODE_STATE_STARTED;
        }
        else if (nodeState_ == NODE_STATE_RECOVER_WAIT_PRIMARY)
        {
            LOG(INFO) << "primary changed while recovery waiting primary";
            if (curr_primary_path_ != self_primary_path_)
            {
                // primary is waiting sync to recovery.
                if (cb_on_recover_wait_primary_)
                    cb_on_recover_wait_primary_();
                nodeState_ = NODE_STATE_STARTED;
            }
            else
            {
                nodeState_ = NODE_STATE_ELECTING;
            }
            LOG(INFO) << "recovery node sync to new primary finished, begin re-enter to the cluster";
            enterClusterAfterRecovery();
        }
        if (isPrimaryWithoutLock())
        {
            nodeState_ = NODE_STATE_ELECTING;
            updateNodeState();
            LOG(INFO) << "begin electing, wait all secondary agree on primary : " << curr_primary_path_;
            checkSecondaryElecting();
        }
        else
        {
            LOG(INFO) << "begin electing, secondary update self to notify new primary : " << curr_primary_path_;
            // notify new primary mine state.
            updateNodeState();
        }
    }
    else if (isPrimaryWithoutLock())
    {
        LOG(WARNING) << "secondary node was deleted : " << path;
        LOG(INFO) << "recheck node for electing or request process";
        checkSecondaryState();        
    }
}

void NodeManagerBase::onDataChanged(const std::string& path)
{
    LOG(INFO) << "node data changed: " << path;
    boost::unique_lock<boost::mutex> lock(mutex_);
    if (path == self_primary_path_ || path == nodePath_)
    {
        return;
    }
    else if (path.find(primaryNodeParentPath_) == std::string::npos)
    {
        LOG(INFO) << "node was deleted, but I did not care : " << path;
        return;
    }
    else if (isPrimaryWithoutLock())
    {
        checkSecondaryState();
    }
    else if (path == curr_primary_path_)
    {
        NodeStateType primary_state = getPrimaryState();
        LOG(INFO) << "current primary node changed : " << primary_state;
        if (nodeState_ == NODE_STATE_STARTED)
        {
            if (primary_state == NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_FINISH_PROCESS)
            {
                LOG(INFO) << "got primary request notify, begin processing on current replica. " << self_primary_path_;
                nodeState_ = NODE_STATE_PROCESSING_REQ_RUNNING;
                ZNode znode;
                std::string sdata;
                std::string packed_reqdata;
                int type;
                if(zookeeper_->getZNodeData(curr_primary_path_, sdata, ZooKeeper::WATCH))
                {
                    znode.loadKvString(sdata);
                    packed_reqdata = znode.getStrValue(ZNode::KEY_PRIMARY_WORKER_REQ_DATA);
                    type = znode.getUInt32Value(ZNode::KEY_REQ_TYPE);
                }
                else
                {
                    LOG(ERROR) << "fatal error, got request data from primary failed while processing request.";
                    LOG(ERROR) << zookeeper_->getErrorString();
                    // maybe primary down
                    if (zookeeper_->getErrorCode() == ZooKeeper::ZERR_ZNONODE)
                    {
                        LOG(ERROR) << "primary node is gone while getting new request data on replica.";
                        return;
                    }
                    // primary is ok, this error can not handle. 
                    throw -1;
                }
                updateNodeState();
                if(cb_on_new_req_from_primary_)
                {
                    if(!cb_on_new_req_from_primary_(type, packed_reqdata))
                    {
                        LOG(ERROR) << "handle request on replica failed, aborting request from replica";
                        nodeState_ = NodeManagerBase::NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY_ABORT;
                        updateNodeState();
                    }
                }
                else
                {
                    LOG(ERROR) << "replica did not have callback on new request from primary.";
                    throw -1;
                }
            }
        }
        else if (nodeState_ == NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY)
        {
            if (primary_state == NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_FINISH_LOG)
            {
                // write request log to local. and ready for new request.
                if (cb_on_wait_primary_)
                    cb_on_wait_primary_();
                nodeState_ = NODE_STATE_STARTED;
                LOG(INFO) << "wait and write request log success on replica: " << self_primary_path_;
                updateNodeState();
            }
            else if (primary_state == NODE_STATE_ELECTING)
            {
                LOG(INFO) << "primary state changed to electing while waiting primary during process request." << self_primary_path_;
                LOG(INFO) << "current request aborted !";
                if (cb_on_abort_request_)
                    cb_on_abort_request_();
                nodeState_ = NODE_STATE_STARTED;
                updateNodeState();
            }
            else if (primary_state == NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_ABORT)
            {
                LOG(INFO) << "primary aborted the request while waiting finish process." << self_primary_path_;
                if (cb_on_abort_request_)
                    cb_on_abort_request_();
                nodeState_ = NODE_STATE_STARTED;
                updateNodeState();
            }
            else
            {
                LOG(INFO) << "wait primary node state not need, ignore";
            }
        }
        else if (nodeState_ == NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY_ABORT)
        {
            if (primary_state == NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_ABORT)
            {
                LOG(INFO) << "primary aborted the request while waiting primary." << self_primary_path_;
                if (cb_on_abort_request_)
                    cb_on_abort_request_();
                nodeState_ = NODE_STATE_STARTED;
                updateNodeState();
            }
            else
            {
                LOG(INFO) << "wait primary node state not need, ignore";
            }
        }
        else if (nodeState_ == NODE_STATE_RECOVER_WAIT_PRIMARY)
        {
            if(primary_state == NODE_STATE_RECOVER_WAIT_REPLICA_FINISH ||
                primary_state == NODE_STATE_ELECTING)
            {
                LOG(INFO) << "wait sync primary success or primary is changed while recovering." << self_primary_path_;
                // primary is waiting sync to recovery.
                if (cb_on_recover_wait_primary_)
                    cb_on_recover_wait_primary_();
                LOG(INFO) << "begin re-enter to the cluster after sync to new primary";
                nodeState_ = NODE_STATE_STARTED;
                enterClusterAfterRecovery();
            }
        }
    }
}

void NodeManagerBase::onChildrenChanged(const std::string& path)
{
    LOG(INFO) << "children changed: " << path;
    if (path == primaryNodeParentPath_ && isPrimaryWithoutLock())
    {
        boost::unique_lock<boost::mutex> lock(mutex_);
        checkSecondaryState();        
    }
}

void NodeManagerBase::checkSecondaryState()
{
    switch(nodeState_)
    {
    case NODE_STATE_ELECTING:
        checkSecondaryElecting();
        break;
    case NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_ABORT:
        checkSecondaryReqAbort();
        break;
    case NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_FINISH_PROCESS:
        checkSecondaryReqProcess();
        break;
    case NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_FINISH_LOG:
        checkSecondaryReqFinishLog();
        break;
    case NODE_STATE_RECOVER_WAIT_REPLICA_FINISH:
    case NODE_STATE_STARTED:
        checkSecondaryRecovery();
        break;
    default:
        break;
    }
}

void NodeManagerBase::checkSecondaryElecting()
{
    if (nodeState_ != NODE_STATE_ELECTING)
    {
        LOG(INFO) << "only in electing state, check secondary need. state: " << nodeState_;
        return;
    }
    std::vector<std::string> node_list;
    zookeeper_->getZNodeChildren(primaryNodeParentPath_, node_list, ZooKeeper::WATCH);
    LOG(INFO) << "in electing state found " << node_list.size() << " children in node " << primaryNodeParentPath_;
    bool all_secondary_ready = true;
    for (size_t i = 0; i < node_list.size(); ++i)
    {
        if (node_list[i] == curr_primary_path_)
            continue;
        NodeStateType state = getNodeState(node_list[i]);
        if (state != NODE_STATE_STARTED && state != NODE_STATE_UNKNOWN)
        {
            all_secondary_ready = false;
            LOG(INFO) << "one secondary node not ready during electing: " <<
                node_list[i] << ", state: " << state << ", keep waiting.";
            break;
        }
    }
    if (all_secondary_ready)
    {
        LOG(INFO) << "all secondary ready, ending electing, primary is ready for new request: " << curr_primary_path_;
        nodeState_ = NODE_STATE_STARTED;
        if (cb_on_elect_finished_)
            cb_on_elect_finished_();
    }
    updateNodeState();
}

// for out caller
void NodeManagerBase::setNodeState(NodeStateType state)
{
    boost::unique_lock<boost::mutex> lock(mutex_);
    if (getPrimaryState() == NODE_STATE_ELECTING)
    {
        if (state == NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY ||
            state == NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY_ABORT)
        {
            if (cb_on_abort_request_)
                cb_on_abort_request_();
            if (curr_primary_path_ != self_primary_path_)
                state = NODE_STATE_STARTED;
            LOG(INFO) << "change state to waiting primary while primary is electing : " << self_primary_path_;
            LOG(INFO) << "It means the old primary is down and new primary is upping, so we must abort the last request process";
        }
        else if(state == NODE_STATE_RECOVER_WAIT_PRIMARY)
        {
            LOG(INFO) << "change state to recovery wait primary while new primary electing : " << self_primary_path_;
            if (curr_primary_path_ != self_primary_path_)
            {
                if (cb_on_recover_wait_primary_)
                    cb_on_recover_wait_primary_();
                nodeState_ = NODE_STATE_STARTED;
            }
            else
            {
                LOG(INFO) << "I become primary while waiting recovery finish.";
                nodeState_ = NODE_STATE_ELECTING;
            }
            enterClusterAfterRecovery();
            return;
        }
    }
    nodeState_ = state;
    updateNodeState();
}

void NodeManagerBase::updateNodeState()
{
    ZNode nodedata;
    setSf1rNodeData(nodedata);
    zookeeper_->setZNodeData(self_primary_path_, nodedata.serialize());
}

void NodeManagerBase::checkSecondaryReqProcess()
{
    if (nodeState_ != NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_FINISH_PROCESS)
    {
        LOG(INFO) << "only in waiting replica state, check secondary request process need. state: " << nodeState_;
        return;
    }
    std::vector<std::string> node_list;
    zookeeper_->getZNodeChildren(primaryNodeParentPath_, node_list, ZooKeeper::WATCH);
    LOG(INFO) << "in request processing state found " << node_list.size() << " children in node " << primaryNodeParentPath_;
    bool all_secondary_ready = true;
    for (size_t i = 0; i < node_list.size(); ++i)
    {
        if (node_list[i] == curr_primary_path_)
            continue;
        NodeStateType state = getNodeState(node_list[i]);
        if (state == NODE_STATE_PROCESSING_REQ_RUNNING)
        {
            all_secondary_ready = false;
            LOG(INFO) << "one secondary node not ready during processing request: " <<
                node_list[i] << ", state: " << state << ", keep waiting.";
            break;
        }
        else if (state == NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY_ABORT)
        {
            all_secondary_ready = false;
            LOG(WARNING) << "request aborted by one replica while waiting finish process. " << node_list[i];
            LOG(INFO) << "begin abort the request on primary and wait all replica to abort it.";
            nodeState_ = NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_ABORT;
            if(cb_on_abort_request_)
                cb_on_abort_request_();
            break;
        }
    }
    if (all_secondary_ready)
    {
        // all replica finished request, wait replica to write log.
        if (cb_on_wait_finish_process_)
            cb_on_wait_finish_process_();
        nodeState_ = NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_FINISH_LOG;
        LOG(INFO) << "all replica finished request, notify and wait replica to write log.";
    }
    updateNodeState();
}

void NodeManagerBase::checkSecondaryReqFinishLog()
{
    if (nodeState_ != NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_FINISH_LOG)
    {
        LOG(INFO) << "only in waiting replica finish log state, check secondary request finish log need. state: " << nodeState_;
        return;
    }
    std::vector<std::string> node_list;
    zookeeper_->getZNodeChildren(primaryNodeParentPath_, node_list, ZooKeeper::WATCH);
    LOG(INFO) << "in request finish log state found " << node_list.size() << " children in node " << primaryNodeParentPath_;
    bool all_secondary_ready = true;
    for (size_t i = 0; i < node_list.size(); ++i)
    {
        if (node_list[i] == curr_primary_path_)
            continue;
        NodeStateType state = getNodeState(node_list[i]);
        if (state == NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY)
        {
            all_secondary_ready = false;
            LOG(INFO) << "one secondary node not ready while waiting finish log: " <<
                node_list[i] << ", state: " << state << ", keep waiting.";
            break;
        }
        else if (state == NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY_ABORT)
        {
            all_secondary_ready = false;
            LOG(WARNING) << "request aborted by one replica when waiting finish log." << node_list[i];
            LOG(INFO) << "begin abort the request on primary and wait all replica to abort it.";
            nodeState_ = NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_ABORT;
            if(cb_on_abort_request_)
                cb_on_abort_request_();
            break;
        }
    }
    if (all_secondary_ready)
    {
        // all replica finished write log
        // ready for next new request.
        if (cb_on_wait_finish_log_)
            cb_on_wait_finish_log_();
        nodeState_ = NODE_STATE_STARTED;
        checkSecondaryRecovery();
        LOG(INFO) << "all replica finished write log, ready for next new request.";
    }
    updateNodeState();
}

void NodeManagerBase::checkSecondaryReqAbort()
{
    if (nodeState_ != NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_ABORT)
    {
        LOG(INFO) << "only in abort state, check secondary abort need. state: " << nodeState_;
        return;
    }
    std::vector<std::string> node_list;
    zookeeper_->getZNodeChildren(primaryNodeParentPath_, node_list, ZooKeeper::WATCH);
    LOG(INFO) << "in abort state found " << node_list.size() << " children in node " << primaryNodeParentPath_;
    bool all_secondary_ready = true;
    for (size_t i = 0; i < node_list.size(); ++i)
    {
        if (node_list[i] == curr_primary_path_)
            continue;
        NodeStateType state = getNodeState(node_list[i]);
        if (state == NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY ||
            state == NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY_ABORT)
        {
            all_secondary_ready = false;
            LOG(INFO) << "one secondary node did not abort while waiting abort request: " <<
                node_list[i] << ", state: " << state << ", keep waiting.";
            break;
        }
    }
    if (all_secondary_ready)
    {
        LOG(INFO) << "all secondary abort request. primary is ready for new request: " << curr_primary_path_;
        nodeState_ = NODE_STATE_STARTED;
        if(cb_on_wait_replica_abort_)
            cb_on_wait_replica_abort_();
    }
    updateNodeState();
}

void NodeManagerBase::checkSecondaryRecovery()
{
    LOG(INFO) << "checking replica recovery state , waiting for sync to primary.";
    std::vector<std::string> node_list;
    zookeeper_->getZNodeChildren(primaryNodeParentPath_, node_list, ZooKeeper::WATCH);
    LOG(INFO) << "checking recovery state found " << node_list.size() << " children in node " << primaryNodeParentPath_;
    bool is_any_recovery_waiting = false;
    for (size_t i = 0; i < node_list.size(); ++i)
    {
        if (node_list[i] == curr_primary_path_)
            continue;
        NodeStateType state = getNodeState(node_list[i]);
        if (state == NODE_STATE_RECOVER_WAIT_PRIMARY)
        {
            is_any_recovery_waiting = true;
            LOG(INFO) << "one secondary node is waiting recovery : " <<
                node_list[i] << ", state: " << state << ", primary need sync this node.";
            break;
        }
    }
    if (is_any_recovery_waiting)
    {
        nodeState_ = NODE_STATE_RECOVER_WAIT_REPLICA_FINISH;
    }
    else
    {
        nodeState_ = NODE_STATE_STARTED;
        if (cb_on_recover_wait_replica_finish_)
            cb_on_recover_wait_replica_finish_();
    }
    updateNodeState();
}

}
