#include "NodeManagerBase.h"
#include "SuperNodeManager.h"
#include "SuperMasterManager.h"
#include "RequestLog.h"
#include "MasterManagerBase.h"
#include "DistributeTest.hpp"
#include "RecoveryChecker.h"

#include <aggregator-manager/MasterNotifier.h>
#include <sstream>
#include <boost/lexical_cast.hpp>


namespace sf1r
{

NodeManagerBase::NodeManagerBase()
    : isDistributionEnabled_(false)
    , nodeState_(NODE_STATE_INIT)
    , masterStarted_(false)
    , processing_step_(0)
    , stopping_(false)
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
        LOG(INFO) << "registering service worker: " << sp_service->getServiceName();
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
    {
        boost::unique_lock<boost::mutex> lock(mutex_);
        stopping_ = true;
    }
    if (masterStarted_)
    {
        stopMasterManager();
        SuperMasterManager::get()->stop();
    }

    leaveCluster();
    zookeeper_->disconnect();
    nodeState_ = NODE_STATE_INIT;
}

void NodeManagerBase::process(ZooKeeperEvent& zkEvent)
{
    LOG (INFO) << CLASSNAME << " worker node event: " << zkEvent.toString();

    if (stopping_)
        return;

    if (zkEvent.type_ == ZOO_SESSION_EVENT && zkEvent.state_ == ZOO_CONNECTED_STATE)
    {
        boost::unique_lock<boost::mutex> lock(mutex_);
        if (nodeState_ == NODE_STATE_STARTING_WAIT_RETRY)
        {
            // retry start
            nodeState_ = NODE_STATE_STARTING;
            enterCluster();
        }
        else
        {
            LOG(WARNING) << "the zookeeper auto reconnected !!! " << self_primary_path_;
            if (nodeState_ == NODE_STATE_RECOVER_RUNNING ||
                nodeState_ == NODE_STATE_PROCESSING_REQ_RUNNING)
            {
                LOG(INFO) << "current node is busy running !!! " << nodeState_;
                return;
            }

            std::string old_primary = curr_primary_path_;
            updateCurrentPrimary();

            if (old_primary != curr_primary_path_)
                LOG(INFO) << "primary changed after auto-reconnect";

            if (old_primary == self_primary_path_)
            {
                if (old_primary != curr_primary_path_)
                {
                    LOG(INFO) << "I lost primary after auto-reconnect" << self_primary_path_;
                    RecoveryChecker::forceExit("I lost primary after auto-reconnect");
                }
                checkSecondaryState(false);
            }
            else 
            {
                checkPrimaryState(old_primary != curr_primary_path_);
                updateNodeState();
            }
        }
    }

    if (zkEvent.type_ == ZOO_SESSION_EVENT && 
        zkEvent.state_ == ZOO_EXPIRED_SESSION_STATE)
    {
        boost::unique_lock<boost::mutex> lock(mutex_);
        // closed by zookeeper because of session expired
        LOG(WARNING) << "worker node disconnected by zookeeper, state: " << zookeeper_->getStateString();
        LOG(WARNING) << "try reconnect : " << sf1rTopology_.curNode_.toString();
        LOG(WARNING) << "before restart, nodeState_ : " << nodeState_;
        if (nodeState_ == NODE_STATE_RECOVER_RUNNING)
        {
            LOG (INFO) << " session expired while recovering, wait recover finish.";
            return;
        }

        // this node is expired, it means disconnect from ZooKeeper for a long time.
        // if any write request not finished, we must abort it.
        if (nodeState_ == NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY ||
            nodeState_ == NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY_ABORT)
        {
            LOG(INFO) << "abort request on current secondary because of expired session.";
            if (cb_on_abort_request_)
                cb_on_abort_request_();
        }
        else if ( nodeState_ == NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_ABORT ||
            nodeState_ == NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_FINISH_PROCESS ||
            nodeState_ == NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_FINISH_LOG)
        {
            LOG(INFO) << "abort request on current primary because of expired session.";
            if(cb_on_wait_replica_abort_)
                cb_on_wait_replica_abort_();
        }
        else if (nodeState_ == NODE_STATE_PROCESSING_REQ_RUNNING)
        {
            LOG (INFO) << " session expired while processing request, wait processing finish.";
            return;
        }

        stopping_ = true;
        self_primary_path_.clear();
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
    zookeeper_->setZNodeData(replicaPath_, ss.str());
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

void NodeManagerBase::setServicesData(ZNode& znode)
{
    std::string services;
    ServiceMapT::const_iterator cit = all_distributed_services_.begin();
    while(cit != all_distributed_services_.end())
    {
        if (services.empty())
            services = cit->first;
        else
            services += "," + cit->first;

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
        znode.setValue(cit->first + ZNode::KEY_COLLECTION, collections);
        ++cit;
    }
    znode.setValue(ZNode::KEY_SERVICE_NAMES, services);
}

void NodeManagerBase::initServices()
{
    ServiceMapT::const_iterator cit = all_distributed_services_.begin();
    while(cit != all_distributed_services_.end())
    {
        cit->second->initWorker();
        ++cit;
    }
}

void NodeManagerBase::setSf1rNodeData(ZNode& znode)
{
    ZNode oldZnode;
    setSf1rNodeData(znode, oldZnode);
}

void NodeManagerBase::setSf1rNodeData(ZNode& znode, ZNode& oldZnode)
{
    std::string olddata;
    if(zookeeper_->getZNodeData(self_primary_path_, olddata, ZooKeeper::WATCH))
    {
        znode.loadKvString(olddata);
        oldZnode = znode;
    }

    znode.setValue(ZNode::KEY_USERNAME, sf1rTopology_.curNode_.userName_);
    znode.setValue(ZNode::KEY_HOST, sf1rTopology_.curNode_.host_);
    znode.setValue(ZNode::KEY_BA_PORT, sf1rTopology_.curNode_.baPort_);
    znode.setValue(ZNode::KEY_DATA_PORT, sf1rTopology_.curNode_.dataPort_);
    znode.setValue(ZNode::KEY_FILESYNC_RPCPORT, (uint32_t)SuperNodeManager::get()->getFileSyncRpcPort());
    znode.setValue(ZNode::KEY_REPLICA_ID, sf1rTopology_.curNode_.replicaId_);
    znode.setValue(ZNode::KEY_NODE_STATE, (uint32_t)nodeState_);
    setServicesData(znode);

    if (nodeState_ == NODE_STATE_STARTED)
        processing_step_ = 0;
    
    znode.setValue(ZNode::KEY_REQ_STEP, processing_step_);
    if (processing_step_ == 0)
    {
        znode.setValue(ZNode::KEY_PRIMARY_WORKER_REQ_DATA, "");
        znode.setValue(ZNode::KEY_REQ_TYPE, (uint32_t)0);
    }

    std::string service_state;
    if (nodeState_ == NODE_STATE_RECOVER_RUNNING || 
        nodeState_ == NODE_STATE_RECOVER_WAIT_PRIMARY ||
        nodeState_ == NODE_STATE_ELECTING ||
        nodeState_ == NODE_STATE_UNKNOWN ||
        nodeState_ == NODE_STATE_PROCESSING_REQ_RUNNING ||
        nodeState_ == NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_ABORT ||
        nodeState_ == NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY_ABORT ||
        nodeState_ == NODE_STATE_INIT ||
        nodeState_ == NODE_STATE_STARTING)
    {
        service_state = "BusyForSelf";
    }
    else
    {
        service_state = "BusyForShard";
        // at least half finished processing, we can get ready to serve read.
        if ((nodeState_ == NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY ||
            nodeState_ == NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_FINISH_PROCESS) &&
            processing_step_ < 100)
        {
            // processing_step_ < 100 means new request is waiting to be totally 
            // accept, less than half nodes confirmed, so the new data will not be available
            // until more than half nodes confirmed the new data.
            // current node is updated and wait more replicas to update to the new data state.
            service_state = "BusyForReplica";
        }
        else if (processing_step_ == 100 &&
            nodeState_ != NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY &&
            nodeState_ != NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_FINISH_PROCESS &&
            nodeState_ != NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_FINISH_LOG)
        {
            // processing_step_ = 100 means new request has been processed by 
            // more than half nodes, so the old data will not be available.
            // current node need update self to the new data state.
            service_state = "BusyForSelf";
        }
        else if (MasterManagerBase::get()->isServiceReadyForRead(false))
        {
            service_state = "ReadyForRead";
        }
    }
    znode.setValue(ZNode::KEY_SERVICE_STATE, service_state);
    DistributeTestSuit::updateMemoryState("NodeState", nodeState_);

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
    LOG(INFO) << "current self node primary path is unregistered: ";
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

    stopping_ = false;
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
    initServices();

    nodeState_ = NODE_STATE_RECOVER_RUNNING;
    updateCurrentPrimary();
    LOG(INFO) << "begin recovering callback : " << self_primary_path_;
    if (cb_on_recovering_)
    {
        // unlock
        mutex_.unlock();
        cb_on_recovering_(start_master);
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
    stopping_ = false;
    if (self_primary_path_.empty() || !zookeeper_->isZNodeExists(self_primary_path_, ZooKeeper::WATCH))
    {
        ZNode znode;
        setSf1rNodeData(znode);
        registerPrimary(znode);

        updateCurrentPrimary();
        if (curr_primary_path_ == self_primary_path_)
        {
            LOG(INFO) << "I enter as primary success." << self_primary_path_;
            nodeState_ = NODE_STATE_ELECTING;
            checkSecondaryElecting(false);
        }
        else
        {
            LOG(INFO) << "enter as primary fail, maybe another node is entering at the same time.";
            if (getPrimaryState() != NODE_STATE_ELECTING)
            {
                updateNodeStateToNewState(NODE_STATE_RECOVER_WAIT_PRIMARY);
                LOG(INFO) << "begin wait new entered primary and try re-enter when sync to new primary.";
                return;
            }
            LOG(INFO) << "new primary is electing, get ready to notify new primary.";
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
    if (!self_primary_path_.empty())
    {
        zookeeper_->deleteZNode(self_primary_path_);
    }
    //self_primary_path_.clear();
 
    zookeeper_->deleteZNode(nodePath_, true);

    std::string replicaPath = replicaPath_;
    std::vector<std::string> childrenList;
    zookeeper_->getZNodeChildren(replicaPath, childrenList, ZooKeeper::NOT_WATCH, false);
    if (childrenList.size() <= 0)
    {
        zookeeper_->deleteZNode(replicaPath);
        zookeeper_->deleteZNode(primaryNodeParentPath_);
    }

    childrenList.clear();
    zookeeper_->getZNodeChildren(topologyPath_, childrenList, ZooKeeper::NOT_WATCH, false);
    if (childrenList.size() <= 0)
    {
        zookeeper_->deleteZNode(topologyPath_);
        zookeeper_->deleteZNode(primaryBasePath_);
        // if no any node, we delete all the remaining unhandled write request.
        zookeeper_->isZNodeExists(ZooKeeperNamespace::getWriteReqQueueParent(), ZooKeeper::NOT_WATCH);
        zookeeper_->deleteZNode(ZooKeeperNamespace::getWriteReqQueueParent(), true);
    }

    childrenList.clear();
    zookeeper_->getZNodeChildren(clusterPath_, childrenList, ZooKeeper::NOT_WATCH, false);
    if (childrenList.size() <= 0)
    {
        zookeeper_->deleteZNode(clusterPath_);
    }
}

bool NodeManagerBase::isOtherPrimaryAvailable()
{
    return !curr_primary_path_.empty() && (curr_primary_path_ != self_primary_path_);
}

bool NodeManagerBase::isPrimary()
{
	if (!isDistributionEnabled_)
		return true;
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

bool NodeManagerBase::canAbortRequest()
{
    // check if abort is allowed, if more than half has finished processing
    // we deny the abort.
    std::vector<std::string> node_list;
    zookeeper_->getZNodeChildren(primaryNodeParentPath_, node_list, ZooKeeper::WATCH);
    LOG(INFO) << "while check if can abort request found " << node_list.size() << " children in node " << primaryNodeParentPath_;
    size_t ready = 0;
    size_t waiting = 0;
    for (size_t i = 0; i < node_list.size(); ++i)
    {
        if (node_list[i] == curr_primary_path_)
        {
            // primary is sure finished before secondary can process.
            ready++;
            continue;
        }
        NodeStateType state = getNodeState(node_list[i]);
        if (state == NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY)
        {
            ready++;
        }
        else if (state == NODE_STATE_STARTED || state == NODE_STATE_PROCESSING_REQ_RUNNING)
        {
            waiting++;
        }
    }
    LOG(INFO) << "while check if can abort request, there are " << ready << " children already finished, and "
        << " there are :" << waiting << " children still waiting.";
    DistributeTestSuit::updateMemoryState("Processing_Ready", ready);
    DistributeTestSuit::updateMemoryState("Processing_Waiting", waiting);
    DistributeTestSuit::updateMemoryState("Processing_Total", node_list.size());
    if (ready >= waiting)
    {
        return false;
    }
    return true;
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
        // check if abort is allowed, if more than half has finished processing
        // we deny the abort.
        if (!canAbortRequest())
        {
            RecoveryChecker::forceExit("exit for deny abort request.");
        }

        setNodeState(NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY_ABORT);
        DistributeTestSuit::testFail(ReplicaFail_At_AbortReq);
    }

}

void NodeManagerBase::finishLocalReqProcess(int type, const std::string& packed_reqdata)
{
    if (isPrimary())
    {
        DistributeTestSuit::testFail(PrimaryFail_At_ReqProcessing);
        boost::unique_lock<boost::mutex> lock(mutex_);
        LOG(INFO) << "send request to other replicas from primary.";
        // write request data to node to notify replica.
        nodeState_ = NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_FINISH_PROCESS;
        ZNode znode;
        setSf1rNodeData(znode);
        znode.setValue(ZNode::KEY_PRIMARY_WORKER_REQ_DATA, packed_reqdata);
        znode.setValue(ZNode::KEY_REQ_TYPE, (uint32_t)type);
        // let the 50% replicas to start processing first.
        // after the 50% finished, we change the step to 100%
        processing_step_ = 50;
        znode.setValue(ZNode::KEY_REQ_STEP, processing_step_);
        zookeeper_->isZNodeExists(self_primary_path_, ZooKeeper::WATCH);
        updateNodeState(znode);

        //std::string oldsdata = znode.serialize();
        //std::string sdata;
        //if(zookeeper_->getZNodeData(self_primary_path_, sdata, ZooKeeper::WATCH))
        //{
        //    znode.loadKvString(sdata);
        //    
        //    std::string packed_reqdata_check = znode.getStrValue(ZNode::KEY_PRIMARY_WORKER_REQ_DATA);
        //    if (packed_reqdata_check != packed_reqdata)
        //    {
        //        LOG(ERROR) << "write request data to ZooKeeper error. data len: " << packed_reqdata_check.size();
        //    }
        //    if (oldsdata != sdata)
        //    {
        //        LOG(ERROR) << "write znode data to ZooKeeper error.";
        //    }
        //    znode.loadKvString(oldsdata);
        //    packed_reqdata_check = znode.getStrValue(ZNode::KEY_PRIMARY_WORKER_REQ_DATA);
        //    if (packed_reqdata_check != packed_reqdata)
        //    {
        //        LOG(ERROR) << "znode serialize and unserialize error.";
        //    }
        //}

        DistributeTestSuit::testFail(PrimaryFail_At_FinishReqLocal);
    }
    else
    {
        DistributeTestSuit::testFail(ReplicaFail_At_ReqProcessing);
        LOG(INFO) << "replica finished local and begin waiting from primary.";
        setNodeState(NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY);
        DistributeTestSuit::testFail(ReplicaFail_At_FinishReqLocal);
    }
}

void NodeManagerBase::onNodeDeleted(const std::string& path)
{
    LOG(INFO) << "node deleted: " << path;
    boost::unique_lock<boost::mutex> lock(mutex_);
    if (path == nodePath_ || path == self_primary_path_)
    {
        LOG(INFO) << "myself node was deleted : " << self_primary_path_;
        if (!stopping_)
        {
            LOG(INFO) << "node was deleted while not stopping: " << self_primary_path_;
            RecoveryChecker::forceExit("node was deleted while not stopping");
        }
    }
    else if (path.find(primaryNodeParentPath_) == std::string::npos)
    {
        LOG(INFO) << "node was deleted, but I did not care : " << self_primary_path_;
    }
    else if (path == curr_primary_path_)
    {
        LOG(WARNING) << "primary node was deleted, myself is : " << self_primary_path_;
        //updateCurrentPrimary();
        checkPrimaryState(true);
    }
    else if (isPrimaryWithoutLock())
    {
        LOG(WARNING) << "secondary node was deleted : " << path;
        LOG(INFO) << "recheck node for electing or request process on " << self_primary_path_;
        checkSecondaryState(false);        
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
        LOG(INFO) << "current primary checking while node changed: " << self_primary_path_;
        checkSecondaryState(path == self_primary_path_ || path == nodePath_);
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
        DistributeTestSuit::loadTestConf();
        checkPrimaryState(false);
    }
}

void NodeManagerBase::onChildrenChanged(const std::string& path)
{
    LOG(INFO) << "children changed: " << path;
    if (path == primaryNodeParentPath_ && isPrimaryWithoutLock())
    {
        boost::unique_lock<boost::mutex> lock(mutex_);
        checkSecondaryState(false);        
    }
}

void NodeManagerBase::checkForPrimaryElecting()
{
    if (stopping_)
        return;
    updateCurrentPrimary();
    switch(nodeState_)
    {
    case NODE_STATE_ELECTING:
        // I am the new primary.
        break;
    case NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY:
    case NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY_ABORT:
        {
            LOG(INFO) << "stop waiting primary for current processing request.";
            if (cb_on_abort_request_)
                cb_on_abort_request_();
            nodeState_ = NODE_STATE_STARTED;
        }
        break;
    case NODE_STATE_RECOVER_WAIT_PRIMARY:
        {
            LOG(INFO) << "primary changed while recovery waiting primary";
            if (curr_primary_path_ != self_primary_path_)
            {
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
        break;
    case NODE_STATE_STARTED:
        break;
    default:
        break;
    }
    if (isPrimaryWithoutLock())
    {
        updateNodeStateToNewState(NODE_STATE_ELECTING);
        LOG(INFO) << "begin electing, wait all secondary agree on primary : " << curr_primary_path_;
        DistributeTestSuit::testFail(PrimaryFail_At_Electing);
        checkSecondaryElecting(true);
    }
    else
    {
        DistributeTestSuit::testFail(ReplicaFail_At_Electing);
        LOG(INFO) << "begin electing, secondary update self to notify new primary : " << curr_primary_path_;
        // notify new primary mine state.
        updateNodeState();
    }
}

void NodeManagerBase::checkPrimaryState(bool primary_deleted)
{
    if (stopping_)
        return;
    // 
    if (primary_deleted)
    {
        checkForPrimaryElecting();
        return;
    }
    NodeStateType primary_state = getPrimaryState();
    LOG(INFO) << "current primary node changed : " << primary_state;
    switch(nodeState_)
    {
    case NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY:
        checkPrimaryForFinishWrite(primary_state);
        break;
    case NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY_ABORT:
        checkPrimaryForAbortWrite(primary_state);
        break;
    case NODE_STATE_RECOVER_WAIT_PRIMARY:
        checkPrimaryForRecovery(primary_state);
        break;
    case NODE_STATE_PROCESSING_REQ_RUNNING:
        break;
    case NODE_STATE_RECOVER_RUNNING:
        break;
    case NODE_STATE_STARTED:
        checkPrimaryForStartNewWrite(primary_state);
        break;
    default:
        break;
    }
}

void NodeManagerBase::checkPrimaryForStartNewWrite(NodeStateType primary_state)
{
    if (primary_state == NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_FINISH_PROCESS)
    {
        LOG(INFO) << "got primary request notify, begin processing on current replica. " << self_primary_path_;
        ZNode znode;
        std::string sdata;
        std::string packed_reqdata;
        int type = 0;
        if(zookeeper_->getZNodeData(curr_primary_path_, sdata, ZooKeeper::WATCH))
        {
            znode.loadKvString(sdata);
            packed_reqdata = znode.getStrValue(ZNode::KEY_PRIMARY_WORKER_REQ_DATA);
            type = znode.getUInt32Value(ZNode::KEY_REQ_TYPE);
            processing_step_ = znode.getUInt32Value(ZNode::KEY_REQ_STEP);
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
            RecoveryChecker::forceExit("exit for unrecovery node state.");
        }
        // check if I can start processing
        if (processing_step_ < 100)
        {
            // only the top half replicas can start at the first step.
            std::vector<std::string> node_list;
            zookeeper_->getZNodeChildren(primaryNodeParentPath_, node_list, ZooKeeper::WATCH);
            LOG(INFO) << "while check if can start request found " << node_list.size() << " children ";
            size_t total = 0;
            size_t myself_pos = 0;
            for (size_t i = 0; i < node_list.size(); ++i)
            {
                if (node_list[i] == curr_primary_path_)
                {
                    // primary is sure finished before secondary can process.
                    total++;
                    continue;
                }
                if (node_list[i] == self_primary_path_)
                    myself_pos = total;
                NodeStateType state = getNodeState(node_list[i]);
                if (state == NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY ||
                    state == NODE_STATE_STARTED ||
                    state == NODE_STATE_PROCESSING_REQ_RUNNING ||
                    state == NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY_ABORT)
                {
                    total++;
                }
            }
            if (myself_pos > total - myself_pos)
            {
                LOG(INFO) << "I am not top half in first step processing, waiting next step. ";
                return;
            }
        }

        updateNodeStateToNewState(NODE_STATE_PROCESSING_REQ_RUNNING);
        DistributeTestSuit::testFail(ReplicaFail_At_ReqProcessing);
        if(cb_on_new_req_from_primary_)
        {
            if(!cb_on_new_req_from_primary_(type, packed_reqdata))
            {
                LOG(ERROR) << "handle request on replica failed, aborting request from replica";
                if (canAbortRequest())
                {
                    updateNodeStateToNewState(NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY_ABORT);
                }
                else
                {
                    RecoveryChecker::forceExit("exit for deny abort request before processing.");
                }
            }
        }
        else
        {
            LOG(ERROR) << "replica did not have callback on new request from primary.";
            RecoveryChecker::forceExit("exit for unrecovery node state.");
        }
    }
}

void NodeManagerBase::checkPrimaryForFinishWrite(NodeStateType primary_state)
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
    else if (primary_state == NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_FINISH_PROCESS)
    {
        if (processing_step_ == 100)
            return;
        ZNode znode;
        std::string sdata;
        if(zookeeper_->getZNodeData(curr_primary_path_, sdata, ZooKeeper::WATCH))
        {
            znode.loadKvString(sdata);
            processing_step_ = znode.getUInt32Value(ZNode::KEY_REQ_STEP);
        }
        if (processing_step_ == 100)
            updateNodeState();
    }
}

void NodeManagerBase::checkPrimaryForAbortWrite(NodeStateType primary_state)
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

void NodeManagerBase::checkPrimaryForRecovery(NodeStateType primary_state)
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

// note : all check is for primary node. and they 
// should be called only in the thread of event handler in ZooKeeper(like onDataChanged)
//  or when start up and stopping. Any other call may cause deadlock.
void NodeManagerBase::checkSecondaryState(bool self_changed)
{
    if (stopping_)
        return;
    switch(nodeState_)
    {
    case NODE_STATE_ELECTING:
        DistributeTestSuit::testFail(PrimaryFail_At_Electing);
        checkSecondaryElecting(self_changed);
        break;
    case NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_ABORT:
        DistributeTestSuit::testFail(PrimaryFail_At_Wait_Replica_Abort);
        checkSecondaryReqAbort(self_changed);
        break;
    case NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_FINISH_PROCESS:
        DistributeTestSuit::testFail(PrimaryFail_At_Wait_Replica_FinishReq);
        checkSecondaryReqProcess(self_changed);
        break;
    case NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_FINISH_LOG:
        DistributeTestSuit::testFail(PrimaryFail_At_Wait_Replica_FinishReqLog);
        checkSecondaryReqFinishLog(self_changed);
        break;
    case NODE_STATE_RECOVER_WAIT_REPLICA_FINISH:
    case NODE_STATE_STARTED:
        DistributeTestSuit::testFail(PrimaryFail_At_Wait_Replica_Recovery);
        checkSecondaryRecovery(self_changed);
        break;
    default:
        break;
    }
}

void NodeManagerBase::checkSecondaryElecting(bool self_changed)
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
    else if(!self_changed)
    {
        updateNodeState();
    }
}

// for out caller
void NodeManagerBase::setNodeState(NodeStateType state)
{
    boost::unique_lock<boost::mutex> lock(mutex_);
    //updateCurrentPrimary();
    NodeStateType primary_state = getPrimaryState();
    if (primary_state == NODE_STATE_ELECTING)
    {
        nodeState_ = state;
        LOG(INFO) << "try to change state to new while electing : " << nodeState_;
        checkForPrimaryElecting();
        return;
    }
    updateNodeStateToNewState(state);
}

void NodeManagerBase::updateNodeStateToNewState(NodeStateType new_state)
{
    if (new_state == nodeState_)
        return;
    NodeStateType oldstate = nodeState_;
    nodeState_ = new_state;
    ZNode nodedata;
    ZNode oldZnode;
    setSf1rNodeData(nodedata, oldZnode);

    bool need_update = nodedata.getStrValue(ZNode::KEY_SERVICE_STATE) != oldZnode.getStrValue(ZNode::KEY_SERVICE_STATE) ||
                  nodedata.getStrValue(ZNode::KEY_SELF_REG_PRIMARY_PATH) != oldZnode.getStrValue(ZNode::KEY_SELF_REG_PRIMARY_PATH);

    zookeeper_->setZNodeData(self_primary_path_, nodedata.serialize());
    if (oldstate == NODE_STATE_STARTED || new_state == NODE_STATE_STARTED || need_update)
    {
        // update to nodepath to make master got notified.
        zookeeper_->setZNodeData(nodePath_, nodedata.serialize());
    }
}

void NodeManagerBase::updateNodeState()
{
    ZNode nodedata;
    setSf1rNodeData(nodedata);
    updateNodeState(nodedata);
}

void NodeManagerBase::updateNodeState(const ZNode& nodedata)
{
    zookeeper_->setZNodeData(self_primary_path_, nodedata.serialize());
    // update to nodepath to make master got notified.
    zookeeper_->setZNodeData(nodePath_, nodedata.serialize());
}

void NodeManagerBase::checkSecondaryReqProcess(bool self_changed)
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

    if (!all_secondary_ready)
    {
        if (!canAbortRequest() &&
            nodeState_ != NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_ABORT)
        {
            LOG(INFO) << "set processing step to 100." << nodeState_;
            processing_step_ = 100;
        }
        if(!self_changed)
        {
            LOG(INFO) << "update processing step ." << nodeState_ << ", step " << processing_step_;
            updateNodeState();
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

void NodeManagerBase::checkSecondaryReqFinishLog(bool self_changed)
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
        checkSecondaryRecovery(self_changed);
        LOG(INFO) << "all replica finished write log, ready for next new request.";
    }
    else if (!self_changed)
    {
        updateNodeState();
    }
}

void NodeManagerBase::checkSecondaryReqAbort(bool self_changed)
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
            state == NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY_ABORT ||
            state == NODE_STATE_PROCESSING_REQ_RUNNING)
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
    else if (!self_changed)
    {
        updateNodeState();
    }
}

void NodeManagerBase::checkSecondaryRecovery(bool self_changed)
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
        if (!self_changed)
        {
            nodeState_ = NODE_STATE_RECOVER_WAIT_REPLICA_FINISH;
            updateNodeState();
        }
        else
            updateNodeStateToNewState(NODE_STATE_RECOVER_WAIT_REPLICA_FINISH);
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
