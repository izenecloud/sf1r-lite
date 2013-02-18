#include "NodeManagerBase.h"
#include "SuperNodeManager.h"
#include "SuperMasterManager.h"
#include "RequestLog.h"
#include "MasterManagerBase.h"
#include "DistributeTest.hpp"

#include <aggregator-manager/MasterNotifier.h>
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

void NodeManagerBase::setZNodePaths()
{
    clusterPath_ = ZooKeeperNamespace::getSF1RClusterPath();
    topologyPath_ = ZooKeeperNamespace::getTopologyPath();
    replicaPath_ = ZooKeeperNamespace::getReplicaPath(sf1rTopology_.curNode_.replicaId_);
    nodePath_ = ZooKeeperNamespace::getNodePath(sf1rTopology_.curNode_.replicaId_, sf1rTopology_.curNode_.nodeId_);
    primaryBasePath_ = ZooKeeperNamespace::getPrimaryBasePath();
    primaryNodeParentPath_ = ZooKeeperNamespace::getPrimaryNodeParentPath(sf1rTopology_.curNode_.nodeId_);
    primaryNodePath_ = ZooKeeperNamespace::getPrimaryNodePath(sf1rTopology_.curNode_.nodeId_);
}


void NodeManagerBase::init(const DistributedTopologyConfig& distributedTopologyConfig)
{
    isDistributionEnabled_ = distributedTopologyConfig.enabled_;
    sf1rTopology_ = distributedTopologyConfig.sf1rTopology_;

    setZNodePaths();
    MasterManagerBase::get()->enableDistribute(isDistributionEnabled_);
}

void NodeManagerBase::registerDistributeService(boost::shared_ptr<IDistributeService> sp_service,
    bool enable_worker, bool enable_master)
{
    if (enable_worker)
    {
        if (all_distributed_services_.find(sp_service->getServiceName()) != 
            all_distributed_services_.end() )
        {
            LOG(WARNING) << "duplicate service name!!!!!!!";
            throw std::runtime_error("duplicate service!");
        }
        all_distributed_services_[sp_service->getServiceName()] = sp_service;
    }
    MasterManagerBase::get()->registerDistributeServiceMaster(sp_service, enable_master);
}

void NodeManagerBase::startMasterManager()
{
    MasterManagerBase::get()->start();
    // init master service.
}

void NodeManagerBase::stopMasterManager()
{
    MasterManagerBase::get()->stop();
}

void NodeManagerBase::detectMasters()
{
    boost::lock_guard<boost::mutex> lock(MasterNotifier::get()->getMutex());
    MasterNotifier::get()->clear();

    replicaid_t replicaId = sf1rTopology_.curNode_.replicaId_;
    std::vector<std::string> childrenList;
    zookeeper_->getZNodeChildren(ZooKeeperNamespace::getReplicaPath(replicaId), childrenList, ZooKeeper::WATCH); // set watcher
    for (nodeid_t nodeId = 1; nodeId <= sf1rTopology_.nodeNum_; nodeId++)
    {
        std::string data;
        std::string nodePath = ZooKeeperNamespace::getNodePath(replicaId, nodeId);
        if (zookeeper_->getZNodeData(nodePath, data, ZooKeeper::WATCH))
        {
            ZNode znode;
            znode.loadKvString(data);

            // if this sf1r node provides master server
            if (znode.hasKey(ZNode::KEY_MASTER_PORT))
            {
                std::string masterHost = znode.getStrValue(ZNode::KEY_HOST);
                uint32_t masterPort;
                try
                {
                    masterPort = boost::lexical_cast<uint32_t>(znode.getStrValue(ZNode::KEY_MASTER_PORT));
                }
                catch (std::exception& e)
                {
                    LOG (ERROR) << "failed to convert masterPort \"" << znode.getStrValue(ZNode::KEY_BA_PORT)
                        << "\" got from master on node" << nodeId << "@" << masterHost;
                    continue;
                }

                LOG (INFO) << "detected Master " << masterHost << ":" << masterPort;
                MasterNotifier::get()->addMasterAddress(masterHost, masterPort);
            }
        }
    }
}

void NodeManagerBase::start()
{
    if (!isDistributionEnabled_)
    {
        return;
    }

    DistributeTestSuit::loadTestConf();

    boost::unique_lock<boost::mutex> lock(mutex_);
    if (!zookeeper_)
    {
        zookeeper_ = ZooKeeperManager::get()->createClient(this);
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

    {
        boost::unique_lock<boost::mutex> lock(mutex_);
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
            LOG(WARNING) << "before restart, nodeState_ : " << nodeState_;
            if (nodeState_ == NODE_STATE_RECOVER_RUNNING)
            {
                LOG (INFO) << " session expired while recovering, wait recover finish.";
                return;
            }
            zookeeper_->disconnect();
            nodeState_ = NODE_STATE_STARTING;
            enterCluster(false);
            LOG (WARNING) << " restarted in NodeManagerBase for ZooKeeper Service finished";
        }
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

void NodeManagerBase::createServiceNodes()
{
    if (!zookeeper_->isZNodeExists(nodePath_, ZooKeeper::WATCH))
    {
        LOG(ERROR) << "current node path not exist while write service node data.";
        return;
    }
    ServiceMapT::const_iterator cit = all_distributed_services_.begin();
    while(cit != all_distributed_services_.end())
    {
        cit->second->initWorker();

        std::string collections;
        std::vector<MasterCollection>& collectionList = sf1rTopology_.curNode_.master_.masterServices_[cit->first].collectionList_;
        for (std::vector<MasterCollection>::iterator it = collectionList.begin();
                it != collectionList.end(); it++)
        {
            if (collections.empty())
                collections = (*it).name_;
            else
                collections += "," + (*it).name_;
        }
        ZNode znode;
        znode.setValue(ZNode::KEY_COLLECTION, collections);
        zookeeper_->createZNode(nodePath_ + "/" + cit->first, znode.serialize(), ZooKeeper::ZNODE_EPHEMERAL);
        ++cit;
    }
}

void NodeManagerBase::setSf1rNodeData(ZNode& znode)
{
    znode.setValue(ZNode::KEY_USERNAME, sf1rTopology_.curNode_.userName_);
    znode.setValue(ZNode::KEY_HOST, sf1rTopology_.curNode_.host_);
    znode.setValue(ZNode::KEY_BA_PORT, sf1rTopology_.curNode_.baPort_);
    znode.setValue(ZNode::KEY_DATA_PORT, sf1rTopology_.curNode_.dataPort_);
    znode.setValue(ZNode::KEY_FILESYNC_RPCPORT, (uint32_t)SuperNodeManager::get()->getFileSyncRpcPort());
    znode.setValue(ZNode::KEY_REPLICA_ID, sf1rTopology_.curNode_.replicaId_);
    znode.setValue(ZNode::KEY_NODE_STATE, (uint32_t)nodeState_);

    // put the registered sequence primary node path to 
    // current node so that the other node can tell 
    // which primary path current node belong to.
    znode.setValue(ZNode::KEY_SELF_REG_PRIMARY_PATH, self_primary_path_);

    if (sf1rTopology_.curNode_.worker_.hasAnyService())
    {
        znode.setValue(ZNode::KEY_WORKER_PORT, sf1rTopology_.curNode_.worker_.port_);
    }

    if (sf1rTopology_.curNode_.master_.hasAnyService())
    {
        znode.setValue(ZNode::KEY_MASTER_PORT, sf1rTopology_.curNode_.master_.port_);
        znode.setValue(ZNode::KEY_MASTER_NAME, sf1rTopology_.curNode_.master_.name_);
    }
}

bool NodeManagerBase::getAllReplicaInfo(std::vector<std::string>& replicas, bool includeprimary)
{
    if (!isDistributionEnabled_)
        return true;
    size_t start_node = 1;
    if (includeprimary)
        start_node = 0;
    std::vector<std::string> node_list;
    zookeeper_->getZNodeChildren(primaryNodeParentPath_, node_list, ZooKeeper::WATCH);
    if (node_list.size() <= start_node)
    {
        // no replica for this node.
        return true;
    }
    std::string sdata;
    replicas.clear();
    std::string ip;
    for (size_t i = start_node; i < node_list.size(); ++i)
    {
        if (zookeeper_->getZNodeData(node_list[i], sdata, ZooKeeper::WATCH))
        {
            replicas.push_back(ip);
            ZNode node;
            node.loadKvString(sdata);
            replicas.back() = node.getStrValue(ZNode::KEY_HOST);
        }
    }
    return true;
}

bool NodeManagerBase::getCurrNodeSyncServerInfo(std::string& ip, int randnum)
{
    if (!isDistributionEnabled_)
        return false;
    std::vector<std::string> node_list;
    zookeeper_->getZNodeChildren(primaryNodeParentPath_, node_list, ZooKeeper::WATCH);
    if (node_list.empty())
    {
        return false;
    }
    int selected = randnum % node_list.size();
    for (size_t i = selected; i <= node_list.size(); ++i)
    {
        if (i == node_list.size())
        {
            // using primary 
            i = 0;
        }
        std::string data;
        if(!zookeeper_->getZNodeData(node_list[i], data, ZooKeeper::WATCH))
        {
            LOG(INFO) << "get node data failed while get filesync server, try next : " << node_list[i];
            continue;
        }
        ZNode node;
        node.loadKvString(data);

        if (i == 0 || node.getUInt32Value(ZNode::KEY_NODE_STATE) == NODE_STATE_STARTED)
        {
            ip = node.getStrValue(ZNode::KEY_HOST);
            return true;
        }
    }
    return false;
}

void NodeManagerBase::updateCurrentPrimary()
{
    std::vector<std::string> primaryList;
    zookeeper_->getZNodeChildren(primaryNodeParentPath_, primaryList, ZooKeeper::WATCH);
    if (primaryList.empty())
    {
        LOG(INFO) << "primary is empty";
        curr_primary_path_.clear();
        DistributeTestSuit::updateMemoryState("IsMinePrimary", 0);
        return;
    }
    curr_primary_path_ = primaryList[0];
    LOG(INFO) << "current primary is : " << curr_primary_path_;
    DistributeTestSuit::updateMemoryState("IsMinePrimary", self_primary_path_ == curr_primary_path_?1:0);
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
    if (!zookeeper_->isZNodeExists(primaryNodeParentPath_, ZooKeeper::WATCH))
    {
        zookeeper_->createZNode(primaryNodeParentPath_);
    }
    if (!self_primary_path_.empty() && zookeeper_->isZNodeExists(self_primary_path_))
    {
        LOG(WARNING) << "the node has register already, reregister will leaving duplicate primary node. old register path: " << self_primary_path_;
        LOG(WARNING) << "delete old primary and register new path";
        unregisterPrimary();
    }
    if (zookeeper_->createZNode(primaryNodePath_, znode.serialize(), ZooKeeper::ZNODE_EPHEMERAL_SEQUENCE))
    {
        self_primary_path_ = zookeeper_->getLastCreatedNodePath();
        zookeeper_->isZNodeExists(self_primary_path_, ZooKeeper::WATCH);
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
    createServiceNodes();

    nodeState_ = NODE_STATE_RECOVER_RUNNING;
    updateCurrentPrimary();
    LOG(INFO) << "begin recovering callback : " << self_primary_path_;
    if (cb_on_recovering_)
    {
        // unlock
        mutex_.unlock();
        cb_on_recovering_();
        DistributeTestSuit::testFail(ReplicaFail_At_Recovering);
        // relock
        mutex_.lock();
    }

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
    if (self_primary_path_.empty() || !zookeeper_->isZNodeExists(self_primary_path_, ZooKeeper::WATCH))
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
            updateNodeStateToNewState(NODE_STATE_RECOVER_WAIT_PRIMARY);
            LOG(INFO) << "begin wait new entered primary and try re-enter when sync to new primary.";
            return;
        }
    }

    LOG(INFO) << "recovery finished. Begin enter cluster after recovery";
    updateNodeState();
    updateCurrentPrimary();

    DistributeTestSuit::testFail(Fail_At_AfterEnterCluster);

    Sf1rNode& curNode = sf1rTopology_.curNode_;
    LOG (INFO) << CLASSNAME
               << " started, cluster[" << sf1rTopology_.clusterId_
               << "] replica[" << curNode.replicaId_
               << "] node[" << curNode.nodeId_
               << "]{"
               << (curNode.worker_.hasAnyService() ?
                       (std::string("worker") + boost::lexical_cast<std::string>(curNode.nodeId_) + " ") : "")
               << curNode.userName_ << "@" << curNode.host_ << "}";


    if(start_master)
    {
        // Start Master manager
        if (sf1rTopology_.curNode_.master_.hasAnyService())
        {
            startMasterManager();
            SuperMasterManager::get()->init(sf1rTopology_);
            SuperMasterManager::get()->start();
            masterStarted_ = true;
        }
    }

    if (sf1rTopology_.curNode_.worker_.hasAnyService())
    {
        detectMasters();
    }
}

void NodeManagerBase::leaveCluster()
{
    unregisterPrimary();
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
    if (isPrimaryWithoutLock())
        DistributeTestSuit::testFail(PrimaryFail_At_BeginReqProcess);
    else
        DistributeTestSuit::testFail(ReplicaFail_At_BeginReqProcess);
}

void NodeManagerBase::notifyMasterReadyForNew()
{
    setNodeState(NODE_STATE_STARTED);
    if (isPrimaryWithoutLock())
        DistributeTestSuit::testFail(PrimaryFail_At_NotifyMasterReadyForNew);
}

void NodeManagerBase::abortRequest()
{
    // notify abort and wait other aborting.
    if (isPrimary())
    {
        LOG(INFO) << "primary abort the request.";
        setNodeState(NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_ABORT);
        DistributeTestSuit::testFail(PrimaryFail_At_AbortReq);
    }
    else
    {
        LOG(INFO) << "replica abort the request.";
        setNodeState(NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY_ABORT);
        DistributeTestSuit::testFail(ReplicaFail_At_AbortReq);
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
        zookeeper_->isZNodeExists(self_primary_path_, ZooKeeper::WATCH);
        zookeeper_->setZNodeData(self_primary_path_, znode.serialize());
        DistributeTestSuit::updateMemoryState("NodeState", nodeState_);
        DistributeTestSuit::testFail(PrimaryFail_At_FinishReqLocal);
    }
    else
    {
        LOG(INFO) << "replica finished local and begin waiting from primary.";
        setNodeState(NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY);
        DistributeTestSuit::testFail(ReplicaFail_At_FinishReqLocal);
    }
}

void NodeManagerBase::onNodeDeleted(const std::string& path)
{
    boost::unique_lock<boost::mutex> lock(mutex_);
    if (path == nodePath_ || path == self_primary_path_)
    {
        LOG(INFO) << "myself node was deleted : " << path;
    }
    else if (path.find(primaryNodeParentPath_) == std::string::npos)
    {
        LOG(INFO) << "node was deleted, but I did not care : " << path;
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
            updateNodeStateToNewState(NODE_STATE_ELECTING);
            LOG(INFO) << "begin electing, wait all secondary agree on primary : " << curr_primary_path_;
            DistributeTestSuit::testFail(PrimaryFail_At_Electing);
            checkSecondaryElecting();
        }
        else
        {
            DistributeTestSuit::testFail(ReplicaFail_At_Electing);
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
    // for primary, need handle the self data changed event.
    // because there may only one node in distribute system.
    if (isPrimaryWithoutLock())
    {
        checkSecondaryState();
    }
    else if (path == self_primary_path_ || path == nodePath_)
    {
        LOG(INFO) << "myself node was changed: " << path;
    }
    else if (path.find(primaryNodeParentPath_) == std::string::npos)
    {
        LOG(INFO) << "node data changed, but I did not care : " << path;
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
                    throw std::runtime_error("exit for unrecovery node state.");
                }
                updateNodeStateToNewState(NODE_STATE_PROCESSING_REQ_RUNNING);
                DistributeTestSuit::testFail(ReplicaFail_At_ReqProcessing);
                if(cb_on_new_req_from_primary_)
                {
                    if(!cb_on_new_req_from_primary_(type, packed_reqdata))
                    {
                        LOG(ERROR) << "handle request on replica failed, aborting request from replica";
                        updateNodeStateToNewState(NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY_ABORT);
                    }
                }
                else
                {
                    LOG(ERROR) << "replica did not have callback on new request from primary.";
                    throw std::runtime_error("exit for unrecovery node state.");
                }
            }
        }
        else if (nodeState_ == NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY)
        {
            DistributeTestSuit::testFail(ReplicaFail_At_Waiting_Primary);
            if (primary_state == NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_FINISH_LOG)
            {
                // write request log to local. and ready for new request.
                if (cb_on_wait_primary_)
                    cb_on_wait_primary_();
                LOG(INFO) << "wait and write request log success on replica: " << self_primary_path_;
                updateNodeStateToNewState(NODE_STATE_STARTED);
            }
            else if (primary_state == NODE_STATE_ELECTING)
            {
                LOG(INFO) << "primary state changed to electing while waiting primary during process request." << self_primary_path_;
                LOG(INFO) << "current request aborted !";
                if (cb_on_abort_request_)
                    cb_on_abort_request_();
                updateNodeStateToNewState(NODE_STATE_STARTED);
            }
            else if (primary_state == NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_ABORT)
            {
                LOG(INFO) << "primary aborted the request while waiting finish process." << self_primary_path_;
                if (cb_on_abort_request_)
                    cb_on_abort_request_();
                updateNodeStateToNewState(NODE_STATE_STARTED);
            }
            else
            {
                LOG(INFO) << "wait primary node state not need, ignore";
            }
        }
        else if (nodeState_ == NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY_ABORT)
        {
            DistributeTestSuit::testFail(ReplicaFail_At_Waiting_Primary_Abort);
            if (primary_state == NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_ABORT)
            {
                LOG(INFO) << "primary aborted the request while waiting primary." << self_primary_path_;
                if (cb_on_abort_request_)
                    cb_on_abort_request_();
                updateNodeStateToNewState(NODE_STATE_STARTED);
            }
            else
            {
                LOG(INFO) << "wait primary node state not need, ignore";
            }
        }
        else if (nodeState_ == NODE_STATE_RECOVER_WAIT_PRIMARY)
        {
            DistributeTestSuit::testFail(ReplicaFail_At_Waiting_Recovery);
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

// note : all check is for primary node. and they 
// should be called only in the thread of event handler in ZooKeeper(like onDataChanged)
//  or when start up and stopping. Any other call may cause deadlock.
void NodeManagerBase::checkSecondaryState()
{
    switch(nodeState_)
    {
    case NODE_STATE_ELECTING:
        DistributeTestSuit::testFail(PrimaryFail_At_Electing);
        checkSecondaryElecting();
        break;
    case NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_ABORT:
        DistributeTestSuit::testFail(PrimaryFail_At_Wait_Replica_Abort);
        checkSecondaryReqAbort();
        break;
    case NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_FINISH_PROCESS:
        DistributeTestSuit::testFail(PrimaryFail_At_Wait_Replica_FinishReq);
        checkSecondaryReqProcess();
        break;
    case NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_FINISH_LOG:
        DistributeTestSuit::testFail(PrimaryFail_At_Wait_Replica_FinishReqLog);
        checkSecondaryReqFinishLog();
        break;
    case NODE_STATE_RECOVER_WAIT_REPLICA_FINISH:
    case NODE_STATE_STARTED:
        DistributeTestSuit::testFail(PrimaryFail_At_Wait_Replica_Recovery);
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
        NodeStateType state = getNodeState(node_list[i]);
        if (node_list[i] == curr_primary_path_)
            continue;
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
        if (cb_on_elect_finished_)
            cb_on_elect_finished_();
        updateNodeStateToNewState(NODE_STATE_STARTED);
    }
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
            // 
            return;
        }
    }
    updateNodeStateToNewState(state);
}

void NodeManagerBase::updateNodeStateToNewState(NodeStateType new_state)
{
    if (new_state == nodeState_)
        return;
    nodeState_ = new_state;
    updateNodeState();
}

void NodeManagerBase::updateNodeState()
{
    ZNode nodedata;
    setSf1rNodeData(nodedata);
    zookeeper_->setZNodeData(self_primary_path_, nodedata.serialize());
    // update to nodepath to make master got notified.
    zookeeper_->setZNodeData(nodePath_, nodedata.serialize());

    DistributeTestSuit::updateMemoryState("NodeState", nodeState_);
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
        NodeStateType state = getNodeState(node_list[i]);
        if (node_list[i] == curr_primary_path_)
            continue;
        if (state == NODE_STATE_PROCESSING_REQ_RUNNING || state == NODE_STATE_STARTED)
        {
            all_secondary_ready = false;
            LOG(INFO) << "one secondary node has not finished during processing request: " <<
                node_list[i] << ", state: " << state << ", keep waiting.";
            break;
        }
        else if (state == NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY_ABORT)
        {
            all_secondary_ready = false;
            LOG(WARNING) << "request aborted by one replica while waiting finish process. " << node_list[i];
            LOG(INFO) << "begin abort the request on primary and wait all replica to abort it.";
            updateNodeStateToNewState(NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_ABORT);
            //if(cb_on_abort_request_)
            //    cb_on_abort_request_();
            break;
        }
    }
    if (all_secondary_ready)
    {
        // all replica finished request, wait replica to write log.
        if (cb_on_wait_finish_process_)
            cb_on_wait_finish_process_();
        updateNodeStateToNewState(NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_FINISH_LOG);
        LOG(INFO) << "all replica finished request, notify and wait replica to write log.";
    }
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
        NodeStateType state = getNodeState(node_list[i]);

        if (node_list[i] == curr_primary_path_)
            continue;
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
            updateNodeStateToNewState(NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_ABORT);
            //if(cb_on_abort_request_)
            //    cb_on_abort_request_();
            break;
        }
    }
    if (all_secondary_ready)
    {
        // all replica finished write log
        // ready for next new request.
        if (cb_on_wait_finish_log_)
            cb_on_wait_finish_log_();
        //updateNodeStateToNewState(NODE_STATE_STARTED);
        checkSecondaryRecovery();
        LOG(INFO) << "all replica finished write log, ready for next new request.";
    }
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
        NodeStateType state = getNodeState(node_list[i]);

        if (node_list[i] == curr_primary_path_)
            continue;
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
        if(cb_on_wait_replica_abort_)
            cb_on_wait_replica_abort_();
        updateNodeStateToNewState(NODE_STATE_STARTED);
    }
    else
    {
        updateNodeState();
    }
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
        NodeStateType state = getNodeState(node_list[i]);

        if (node_list[i] == curr_primary_path_)
            continue;
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
        // 
        nodeState_ = NODE_STATE_RECOVER_WAIT_REPLICA_FINISH;
        updateNodeState();
    }
    else
    {
        NodeStateType new_state = NODE_STATE_STARTED;
        if (cb_on_recover_wait_replica_finish_)
            cb_on_recover_wait_replica_finish_();
        updateNodeStateToNewState(new_state);
    }
}

}
