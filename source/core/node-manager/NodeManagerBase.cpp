#include "NodeManagerBase.h"
#include "SuperNodeManager.h"
#include "SuperMasterManager.h"
#include "MasterManagerBase.h"
#include "DistributeTest.hpp"
#include "RecoveryChecker.h"

#include <aggregator-manager/MasterNotifier.h>
#include <sstream>
#include <boost/lexical_cast.hpp>

static const bool s_enable_async_ = true;

namespace sf1r
{

bool NodeManagerBase::isAsyncEnabled()
{
    return s_enable_async_;
}

NodeManagerBase::NodeManagerBase()
    : isDistributionEnabled_(false)
    , nodeState_(NODE_STATE_INIT)
    , masterStarted_(false)
    , processing_step_(0)
    , stopping_(false)
    , need_stop_(false)
    , slow_write_running_(false)
    , need_check_electing_(false)
    , need_check_recover_(true)
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
    LOG(INFO) << "node starting mode : " << s_enable_async_;
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
    masterStarted_ =  false;
}

void NodeManagerBase::detectMasters()
{
    boost::lock_guard<boost::mutex> lock(MasterNotifier::get()->getMutex());
    MasterNotifier::get()->clear();

    replicaid_t replicaId = sf1rTopology_.curNode_.replicaId_;
    std::vector<std::string> children;
    std::string serverParentPath = ZooKeeperNamespace::getServerParentPath();
    zookeeper_->getZNodeChildren(serverParentPath, children, ZooKeeper::WATCH);
    for (size_t i = 0; i < children.size(); ++i)
    {
        std::string data;
        if (zookeeper_->getZNodeData(children[i], data, ZooKeeper::WATCH))
        {
            ZNode znode;
            znode.loadKvString(data);
            // if this sf1r node provides master server
            if (znode.hasKey(ZNode::KEY_MASTER_PORT) &&
                znode.getUInt32Value(ZNode::KEY_REPLICA_ID) == replicaId)
            {
                std::string masterHost = znode.getStrValue(ZNode::KEY_HOST);
                uint32_t masterPort;
                try
                {
                    masterPort = boost::lexical_cast<uint32_t>(znode.getStrValue(ZNode::KEY_MASTER_PORT));
                }
                catch (std::exception& e)
                {
                    LOG (ERROR) << "failed to convert masterPort \"" << znode.getStrValue(ZNode::KEY_MASTER_PORT)
                        << "\" got from master on node" << children[i] << "@" << masterHost;
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

    if (!zookeeper_)
    {
        zookeeper_ = ZooKeeperManager::get()->createClient(this, true);
    }
    else
    {
        zookeeper_->connect(true);
    }
    if (!zookeeper_->isConnected())
    {
        LOG(INFO) << "still waiting zookeeper";
        nodeState_ = NODE_STATE_STARTING_WAIT_RETRY;
        return;
    }
    boost::unique_lock<boost::mutex> lock(mutex_);
    if (nodeState_ == NODE_STATE_INIT)
    {
        nodeState_ = NODE_STATE_STARTING;
        enterCluster();
    }
}

void NodeManagerBase::notifyStop()
{
    LOG(INFO) << "========== notify stop ============= ";
    {
        if (mutex_.try_lock())
        {
            LOG(INFO) << "try lock success, we can wait to stop.";
            mutex_.unlock();
        }
        else
        {
            if (nodeState_ == NODE_STATE_STARTING || nodeState_ == NODE_STATE_INIT)
            {
                LOG(INFO) << "stopping while starting. " << nodeState_;
                need_stop_ = true;
                if (zookeeper_)
                {
                    stop();
                    zookeeper_->disconnect();
                }
                return;
            }
        }
        boost::unique_lock<boost::mutex> lock(mutex_);
        need_stop_ = true;
        if (stopping_)
        {
            LOG(INFO) << "already stopped";
            return;
        }

        if ((nodeState_ == NODE_STATE_STARTED ||
             nodeState_ == NODE_STATE_STARTING_WAIT_RETRY ||
             nodeState_ == NODE_STATE_STARTING ||
             nodeState_ == NODE_STATE_INIT ||
             nodeState_ == NODE_STATE_RECOVER_WAIT_PRIMARY ) &&
            !MasterManagerBase::get()->hasAnyCachedRequest())
        {
            stop();
        }
        else
        {
            LOG(INFO) << "waiting the node become idle while stopping ...";
            stop_cond_.wait(lock);
        }
    }
    zookeeper_->disconnect();
    LOG(INFO) << "========== stop finished. ========== ";
}

void NodeManagerBase::stop()
{
    if (stopping_)
        return;
    stopping_ = true;
    LOG(INFO) << "begin stopping on state : " << nodeState_;
    if (masterStarted_)
    {
        stopMasterManager();
        SuperMasterManager::get()->stop();
    }

    leaveCluster();

    nodeState_ = NODE_STATE_INIT;
    stop_cond_.notify_all();
}

void NodeManagerBase::setElectingState()
{
    need_check_electing_ = true;
    if (s_enable_async_)
    {
        cb_on_pause_sync_();
    }
    
    MasterManagerBase::get()->disableNewWrite();
}

std::string NodeManagerBase::findReCreatedSelfPrimaryNode()
{
    std::string new_self_primary;

    if (zookeeper_->isZNodeExists(self_primary_path_, ZooKeeper::WATCH))
    {
        return self_primary_path_;
    }

    std::vector<std::string> childrenList;
    zookeeper_->getZNodeChildren(primaryNodeParentPath_, childrenList, ZooKeeper::WATCH);
    for (size_t i = 0; i < childrenList.size(); ++i)
    {
        std::string sdata;
        zookeeper_->getZNodeData(childrenList[i], sdata, ZooKeeper::WATCH);
        ZNode znode;
        znode.loadKvString(sdata);
        if (znode.getStrValue(ZNode::KEY_HOST) == sf1rTopology_.curNode_.host_)
        {
            LOG(INFO) << "found self primary path for current : " << childrenList[i];
            new_self_primary = childrenList[i];
            zookeeper_->isZNodeExists(new_self_primary, ZooKeeper::WATCH);
            break;
        }
    }

    return new_self_primary;
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
            MasterManagerBase::get()->notifyChangedPrimary(false);
            std::string new_self_primary  = findReCreatedSelfPrimaryNode();
            if (!self_primary_path_.empty() && new_self_primary.empty())
            {
                LOG(WARNING) << "waiting self_primary_path_ re-create :" << self_primary_path_;
                return;
            }
            self_primary_path_ = new_self_primary;
            LOG(WARNING) << "new self_primary_path_ is :" << self_primary_path_;
            stopping_ = true;
            unregisterPrimary();
            // retry start
            nodeState_ = NODE_STATE_STARTING;
            enterCluster(!masterStarted_);
        }
        else if (nodeState_ != NODE_STATE_STARTING &&
                 nodeState_ != NODE_STATE_INIT)
        {
            LOG(WARNING) << "the zookeeper auto reconnected !!! " << self_primary_path_;
            LOG(INFO) << "begin electing for auto-reconnect.";
            checkForPrimaryElecting();
        }
    }

    if (zkEvent.type_ == ZOO_SESSION_EVENT && 
        zkEvent.state_ == ZOO_EXPIRED_SESSION_STATE)
    {
        {
            boost::unique_lock<boost::mutex> lock(mutex_);
            // closed by zookeeper because of session expired
            LOG(WARNING) << "worker node disconnected by zookeeper, state: " << zookeeper_->getStateString();
            LOG(WARNING) << "try reconnect : " << sf1rTopology_.curNode_.toString();
            LOG(WARNING) << "before restart, nodeState_ : " << nodeState_;
            setElectingState();
            while (nodeState_ == NODE_STATE_RECOVER_RUNNING || nodeState_ == NODE_STATE_PROCESSING_REQ_RUNNING)
            {
                LOG (INFO) << " session expired while processing request or recovering, wait processing finish.";
                waiting_reenter_cond_.timed_wait(lock, boost::posix_time::seconds(10));
            }
            // this node is expired, it means disconnect from ZooKeeper for a long time.
            // if any write request not finished, we must abort it.
            resetWriteState();

            MasterManagerBase::get()->notifyChangedPrimary(false);
            stopping_ = true;
            self_primary_path_.clear();
            need_check_electing_ = false;
        }

        zookeeper_->disconnect();
        if (!checkZooKeeperService())
        {
            boost::unique_lock<boost::mutex> lock(mutex_);
            stopping_ = false;
            // process will be resumed after zookeeper recovered
            LOG (WARNING) << " waiting for ZooKeeper Service while expired ...";
            return;
        }

        boost::unique_lock<boost::mutex> lock(mutex_);
        nodeState_ = NODE_STATE_STARTING;
        enterCluster(!masterStarted_);
        LOG (WARNING) << " restarted in NodeManagerBase for ZooKeeper Service finished";
    }
    if (zkEvent.type_ == ZOO_CHILD_EVENT && masterStarted_)
    {
        if (sf1rTopology_.curNode_.worker_.hasAnyService())
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

    //setServicesData(znode);
    if (nodeState_ == NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_FINISH_PROCESS)
    {
        znode.setValue(ZNode::KEY_PRIMARY_WORKER_REQ_DATA, saved_packed_reqdata_);
        znode.setValue(ZNode::KEY_REQ_TYPE, (uint32_t)saved_reqtype_);
    }

    if (nodeState_ == NODE_STATE_STARTED)
    {
        processing_step_ = 0;
        slow_write_running_ = false;
        saved_packed_reqdata_.clear();
        saved_reqtype_ = 0;
    }   
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
        if (!slow_write_running_ && nodeState_ == NODE_STATE_PROCESSING_REQ_RUNNING)
            service_state = "ReadyForRead";
    }
    else
    {
        service_state = "ReadyForRead";
        // at least half finished processing, we can get ready to serve read.
        if ((nodeState_ == NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY ||
            nodeState_ == NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_FINISH_PROCESS) &&
            processing_step_ < 100 && slow_write_running_)
        {
            // processing_step_ < 100 means new request is waiting to be totally 
            // accept, less than half nodes confirmed, so the new data will not be available
            // until more than half nodes confirmed the new data.
            // current node is updated and wait more replicas to update to the new data state.
            service_state = "BusyForReplica";
        }
        else if (slow_write_running_ && processing_step_ == 100 &&
            nodeState_ != NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY &&
            nodeState_ != NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_FINISH_PROCESS &&
            nodeState_ != NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_FINISH_LOG)
        {
            // processing_step_ = 100 means new request has been processed by 
            // more than half nodes, so the old data will not be available.
            // current node need update self to the new data state.
            service_state = "BusyForSelf";
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

bool NodeManagerBase::getCurrPrimaryInfo(std::string& primary_host)
{
    if (!zookeeper_ || !zookeeper_->isConnected())
        return false;

    std::vector<std::string> node_list;
    zookeeper_->getZNodeChildren(primaryNodeParentPath_, node_list, ZooKeeper::WATCH);
    if (node_list.empty())
    {
        // no primary.
        return false;
    }
    primary_host = node_list.front();
    std::string sdata;
    if (zookeeper_->getZNodeData(primary_host, sdata, ZooKeeper::WATCH))
    {
        ZNode node;
        node.loadKvString(sdata);
        uint32_t state = node.getUInt32Value(ZNode::KEY_NODE_STATE);
        if (state == NODE_STATE_ELECTING ||
            state == NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_ABORT ||
            state == NODE_STATE_UNKNOWN)
        {
            if (nodeState_ != NODE_STATE_RECOVER_RUNNING &&
                nodeState_ != NODE_STATE_RECOVER_WAIT_PRIMARY)
            {
                LOG(INFO) << "primary is busy while get primary. " << state;
                primary_host.clear();
                return false;
            }
        }
        primary_host = node.getStrValue(ZNode::KEY_HOST);
        return true;
    }
    primary_host.clear();
    return false;
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
            ZNode node;
            node.loadKvString(sdata);
            uint32_t state = node.getUInt32Value(ZNode::KEY_NODE_STATE);
            if (state == NODE_STATE_RECOVER_RUNNING ||
                state == NODE_STATE_RECOVER_WAIT_PRIMARY ||
                state == NODE_STATE_STARTING_WAIT_RETRY ||
                state == NODE_STATE_STARTING
                )
            {
                LOG(INFO) << "ignore replica for not ready : " << state;
                continue;
            }
            replicas.push_back(ip);
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
            if (i == 0)
                break;
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
        DistributeTestSuit::updateMemoryState("CurrentPrimary", "Empty");
        return;
    }
    curr_primary_path_ = primaryList[0];
    LOG(INFO) << "current primary is : " << curr_primary_path_;
    DistributeTestSuit::updateMemoryState("IsMinePrimary", self_primary_path_ == curr_primary_path_?1:0);
    getPrimaryState();

    std::string primary_ip;
    std::string sdata;
    if (zookeeper_->getZNodeData(curr_primary_path_, sdata, ZooKeeper::WATCH))
    {
        ZNode node;
        node.loadKvString(sdata);
        primary_ip = node.getStrValue(ZNode::KEY_HOST);
        DistributeTestSuit::updateMemoryState("CurrentPrimary", curr_primary_path_ + " : " + primary_ip);
    }
}

void NodeManagerBase::unregisterPrimary()
{
    std::string my_registed_primary = findReCreatedSelfPrimaryNode();
    while(!my_registed_primary.empty())
    {
        zookeeper_->deleteZNode(my_registed_primary);
        my_registed_primary = findReCreatedSelfPrimaryNode();
    }

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

bool NodeManagerBase::registerPrimary(ZNode& znode)
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
        return false;
    }
    return true;
}

void NodeManagerBase::enterCluster(bool start_master)
{
    if (nodeState_ == NODE_STATE_STARTED)
    {
        stopping_ = false;
        return;
    }

    if (nodeState_ == NODE_STATE_RECOVER_RUNNING ||
        nodeState_ == NODE_STATE_RECOVER_WAIT_PRIMARY)
    {
        LOG(WARNING) << "try to reenter cluster while node is recovering!" << sf1rTopology_.curNode_.toString();
        stopping_ = false;
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

            if (znode.getStrValue(ZNode::KEY_HOST) != sf1rTopology_.curNode_.host_)
            {
                throw std::runtime_error(ss.str());
            }
            LOG(WARNING) << " node already existed : " << nodePath_;
        }
        else
        {
            nodeState_ = NODE_STATE_STARTING_WAIT_RETRY;
            LOG (ERROR) << CLASSNAME << " Failed to start (" << zookeeper_->getErrorString()
                << "), waiting for retry ..." << std::endl;
            stopping_ = false;
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
        if (!zookeeper_->isConnected())
        {
            nodeState_ = NODE_STATE_STARTING_WAIT_RETRY;
            LOG(WARNING) << " ZooKeeper lost while enter cluster. wait retry.";
            stopping_ = false;
            return;
        }
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
            updateNodeState(znode);
            stopping_ = false;
            return;
        }
    }
    //nodeState_ = NODE_STATE_STARTED;
    enterClusterAfterRecovery(start_master);
}

void NodeManagerBase::enterClusterAfterRecovery(bool start_master)
{
    stopping_ = false;
    if (self_primary_path_.empty() || !zookeeper_->isZNodeExists(self_primary_path_, ZooKeeper::WATCH))
    {
        if (!zookeeper_->isConnected())
            return;
        ZNode znode;
        setSf1rNodeData(znode);
        if(!registerPrimary(znode))
        {
            LOG(INFO) << "register primary failed. waiting retry.";
            return;
        }

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
            else if (s_enable_async_)
            {
                nodeState_ = NODE_STATE_ELECTING;
                updateSelfPrimaryNodeState();
                return;
            }
            LOG(INFO) << "new primary is electing, get ready to notify new primary.";
        }
    }

    if (!cb_on_recover_check_(curr_primary_path_ == self_primary_path_))
    {
        LOG(WARNING) << "check for log failed after enter cluster, must re-enter.";
        unregisterPrimary();
        nodeState_ = NODE_STATE_STARTING;
        sleep(10);
        if (s_enable_async_)
        {
            reEnterCluster();
            return;
        }
        updateCurrentPrimary();
        setElectingState();
        updateNodeState();
        return;
    }

    nodeState_ = NODE_STATE_STARTED;
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
    if (masterStarted_ && nodeState_ == NODE_STATE_STARTED)
    {
        MasterManagerBase::get()->notifyChangedPrimary(curr_primary_path_ == self_primary_path_ && !self_primary_path_.empty());
        MasterManagerBase::get()->updateMasterReadyForNew(true);
    }
}

void NodeManagerBase::reEnterCluster()
{
    resetWriteState();

    MasterManagerBase::get()->notifyChangedPrimary(false);
    updateCurrentPrimary();
    need_check_electing_ = false;

    LOG(WARNING) << "begin re-enter cluster";
    stopping_ = true;
    unregisterPrimary();
    nodeState_ = NODE_STATE_STARTING;
    enterCluster(!masterStarted_);
}

void NodeManagerBase::leaveCluster()
{
    if (!self_primary_path_.empty())
    {
        zookeeper_->deleteZNode(self_primary_path_);
    }
    //self_primary_path_.clear();
 
    zookeeper_->deleteZNode(nodePath_, true);

    if (!zookeeper_->isConnected())
        return;

    std::string replicaPath = replicaPath_;
    std::vector<std::string> childrenList;
    zookeeper_->getZNodeChildren(replicaPath, childrenList, ZooKeeper::NOT_WATCH, false);
    if (childrenList.size() <= 0)
    {
        zookeeper_->deleteZNode(replicaPath);
    }
    childrenList.clear();

    zookeeper_->getZNodeChildren(primaryNodeParentPath_, childrenList, ZooKeeper::NOT_WATCH, false);
    if (childrenList.size() <= 0)
    {
        zookeeper_->deleteZNode(primaryNodeParentPath_);
        zookeeper_->isZNodeExists(ZooKeeperNamespace::getCurrWriteReqQueueParent(sf1rTopology_.curNode_.nodeId_),
            ZooKeeper::NOT_WATCH);
        zookeeper_->deleteZNode(ZooKeeperNamespace::getCurrWriteReqQueueParent(sf1rTopology_.curNode_.nodeId_), true);
    }
    childrenList.clear();

    zookeeper_->getZNodeChildren(topologyPath_, childrenList, ZooKeeper::NOT_WATCH, false);
    if (childrenList.size() <= 0)
    {
        zookeeper_->deleteZNode(topologyPath_);
        zookeeper_->deleteZNode(primaryBasePath_, true);
        // if no any node, we delete all the remaining unhandled write request.
        zookeeper_->isZNodeExists(ZooKeeperNamespace::getRootWriteReqQueueParent(), ZooKeeper::NOT_WATCH);
        zookeeper_->deleteZNode(ZooKeeperNamespace::getRootWriteReqQueueParent(), true);
        zookeeper_->deleteZNode(ZooKeeperNamespace::getWriteReqPrepareParent(), true);
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
    if (stopping_)
        return false;
    if (curr_primary_path_.empty() || self_primary_path_.empty())
        return false;
    return curr_primary_path_ == self_primary_path_;
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
    MasterManagerBase::get()->endPreparedWrite();
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
    if (s_enable_async_)
    {
        return isPrimaryWithoutLock();
    }
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
    zookeeper_->isZNodeExists(self_primary_path_, ZooKeeper::WATCH);
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
        if (stopping_)
            return;
        LOG(INFO) << "send request to other replicas from primary.";
        // write request data to node to notify replica.
        nodeState_ = NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_FINISH_PROCESS;
        saved_packed_reqdata_ = packed_reqdata;
        saved_reqtype_ = type;
        // let the 50% replicas to start processing first.
        // after the 50% finished, we change the step to 100%
        processing_step_ = 50;
        if (!slow_write_running_)
            processing_step_ = 100;

        if (need_check_electing_ || !zookeeper_->isZNodeExists(self_primary_path_, ZooKeeper::WATCH))
        {
            LOG(WARNING) << "lost connection from ZooKeeper while finish request." << self_primary_path_;
            //checkForPrimaryElecting();
            // update to get notify on event callback and check for electing.
            updateNodeState();
        }
        else
            updateSelfPrimaryNodeState();
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
            LOG(ERROR) << "node was deleted while not stopping: " << self_primary_path_;
            // try re-enter 
            LOG(WARNING) << "need re-enter";
            reEnterCluster();
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
    if (need_check_electing_)
    {
        checkForPrimaryElecting();
        return;
    }
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
        //DistributeTestSuit::loadTestConf();
        checkPrimaryState(false);
    }
}

void NodeManagerBase::onChildrenChanged(const std::string& path)
{
    LOG(INFO) << "children changed: " << path;
    if (path == primaryNodeParentPath_ && isPrimaryWithoutLock())
    {
        boost::unique_lock<boost::mutex> lock(mutex_);
        need_check_recover_ = true;
        checkSecondaryState(false);        
    }
}

void NodeManagerBase::resetWriteState()
{
    switch(nodeState_)
    {
    case NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_FINISH_PROCESS:
        if (s_enable_async_)
        {
            if (cb_on_wait_finish_process_)
                cb_on_wait_finish_process_();
            if (cb_on_wait_finish_log_)
                cb_on_wait_finish_log_();
            nodeState_ = NODE_STATE_STARTED;
            return;
        }
        //break; walk through by intended
    case NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_FINISH_LOG:
        if (s_enable_async_)
        {
            LOG(INFO) << "in async mode, waiting write can finish without abort while electing.";
            if (cb_on_wait_finish_log_)
                cb_on_wait_finish_log_();
            nodeState_ = NODE_STATE_STARTED;
            return;
        }
        //break; walk through by intended
    case NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_ABORT:
        {
            LOG(INFO) << "abort request on current primary because of lost registered primary.";
            if(cb_on_wait_replica_abort_)
                cb_on_wait_replica_abort_();
            nodeState_ = NODE_STATE_STARTED;
        }
        break;
    case NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY:
        if (s_enable_async_)
        {
            LOG(INFO) << "in async mode, waiting write can finish without abort while electing.";
            if (cb_on_wait_primary_)
                cb_on_wait_primary_();
            nodeState_ = NODE_STATE_STARTED;
            return;
        }
        //break; walk through by intended
    case NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY_ABORT:
        {
            LOG(INFO) << "stop waiting primary for current processing request.";
            if (cb_on_abort_request_)
                cb_on_abort_request_();
            nodeState_ = NODE_STATE_STARTED;
        }
        break;
    default:
        break;
    }
}

bool NodeManagerBase::isNeedReEnterCluster()
{
    // starting, no need to re enter. It will check after recover finished.
    if (nodeState_ == NODE_STATE_RECOVER_RUNNING && self_primary_path_.empty())
        return false;

    if (!masterStarted_)
        return true;
    std::string new_self_primary;
    std::string old_primary = curr_primary_path_;
    int retry = 0;
    while (retry++ < 3)
    {
        new_self_primary = findReCreatedSelfPrimaryNode();
        if (!new_self_primary.empty())
            break;
        sleep(2);
    }
    LOG(INFO) << "checking re-enter , new_self_primary is :" << new_self_primary;

    if (new_self_primary.empty() || new_self_primary != self_primary_path_ )
    {
        LOG(WARNING) << "current self primary changed or lost, need re-enter cluster.";
        return true;
    }

    updateCurrentPrimary();
    if (curr_primary_path_.empty() || old_primary != curr_primary_path_)
    {
        // avoid re-enter cluster if my state is ok while primary changed.
        // We just make sure our log is newest.
        if (old_primary == self_primary_path_)
        {
            LOG(WARNING) << "need re-enter cluster for I lost primary." << old_primary << " vs " << curr_primary_path_;
            return true;
        }
        if (nodeState_ != NODE_STATE_STARTED && nodeState_ != NODE_STATE_RECOVER_WAIT_PRIMARY)
            return true;

        if (curr_primary_path_ == self_primary_path_)
        {
            LOG(WARNING) << "I became the new primary." << old_primary << " vs " << curr_primary_path_;
            nodeState_ = NODE_STATE_ELECTING;
        }
    }

    //NodeStateType primary_state = getPrimaryState();
    //if (primary_state == NODE_STATE_ELECTING || primary_state == NODE_STATE_UNKNOWN)
    //{
    //    LOG(WARNING) << "need re-enter cluster for primary is electing ";
    //    return true;
    //}
    if (!cb_on_recover_check_(curr_primary_path_ == self_primary_path_))
    {
        LOG(WARNING) << "need re-enter cluster for request log fall behind.";
        return true;
    }
    LOG(INFO) << "no need re-enter cluster, current state : " << nodeState_;
    return false;
}

void NodeManagerBase::checkForPrimaryElectingInAsyncMode()
{
    LOG(INFO) << "primary is electing current node state: " << nodeState_;
    switch(nodeState_)
    {
    case NODE_STATE_RECOVER_RUNNING:
        break;
    case NODE_STATE_PROCESSING_REQ_RUNNING:
        LOG(INFO) << "check electing wait for idle." << nodeState_;
        setElectingState();
        return;
        break;
    default:
        break;
    }

    if(!self_primary_path_.empty() && findReCreatedSelfPrimaryNode().empty())
    {
        LOG(INFO) << "waiting self_primary_path_ re-created.";
        return;
    }
    need_check_recover_ =  true;
    updateCurrentPrimary();
    if (isPrimaryWithoutLock())
    {
        if (nodeState_ != NODE_STATE_ELECTING)
        {
            setElectingState();
            LOG(INFO) << "notify log sync worker to pause and begin electing.";
            return;
        }
    }
}

// Note: Must be called in ZooKeeper event handler.
void NodeManagerBase::checkForPrimaryElecting()
{
    if (stopping_)
        return;
    if (!zookeeper_ || !zookeeper_->isConnected())
    {
        LOG(INFO) << "ZooKeeper not connected while check for electing.";
        return;
    }

    if (s_enable_async_)
    {
        checkForPrimaryElectingInAsyncMode();
        return;
    }
    LOG(INFO) << "primary is electing current node state: " << nodeState_;
    switch(nodeState_)
    {
    case NODE_STATE_RECOVER_RUNNING:
        break;
    case NODE_STATE_PROCESSING_REQ_RUNNING:
        LOG(INFO) << "check electing wait for idle." << nodeState_;
        setElectingState();
        return;
        break;
    default:
        break;
    }

    if(!self_primary_path_.empty() && findReCreatedSelfPrimaryNode().empty())
    {
        LOG(INFO) << "waiting self_primary_path_ re-created.";
        return;
    }


    need_check_recover_ =  true;

    if (!isNeedReEnterCluster())
    {
        need_check_electing_ = false;
        if (isPrimaryWithoutLock())
        {
            checkSecondaryState(false);
        }
        else
        {
            if (nodeState_ == NODE_STATE_RECOVER_RUNNING)
                return;
            checkPrimaryState(false);
        }
        updateNodeState();
        return;
    }

    if (!zookeeper_->isConnected())
    {
        setElectingState();
        return;
    }

    LOG(WARNING) << "self_primary_path_ lost or log fall behind, need re-enter";
    reEnterCluster();
    return;

    //if (isPrimaryWithoutLock())
    //{
    //    updateNodeStateToNewState(NODE_STATE_ELECTING);
    //    LOG(INFO) << "begin electing, wait all secondary agree on primary : " << curr_primary_path_;
    //    DistributeTestSuit::testFail(PrimaryFail_At_Electing);
    //    checkSecondaryElecting(true);
    //}
    //else
    //{
    //    nodeState_ = NODE_STATE_STARTED;
    //    DistributeTestSuit::testFail(ReplicaFail_At_Electing);
    //    LOG(INFO) << "begin electing, secondary update self to notify new primary : " << curr_primary_path_;
    //    // notify new primary mine state.
    //    updateNodeState();
    //}
}

bool NodeManagerBase::checkElectingInAsyncMode(uint32_t last_reqid)
{
    boost::unique_lock<boost::mutex> lock(mutex_);
    if (stopping_ || !zookeeper_ || !zookeeper_->isConnected())
        return false;
    updateCurrentPrimary();
    if (getPrimaryState() == NODE_STATE_ELECTING || need_check_electing_)
    {
        LOG(INFO) << "in check point, electing needed, ready to electing on current" << self_primary_path_;
        resetWriteState();
        nodeState_ = NODE_STATE_ELECTING;
        need_check_electing_ = false;
        ZNode nodedata;
        setSf1rNodeData(nodedata);
        nodedata.setValue(ZNode::KEY_LAST_WRITE_REQID, last_reqid);
        updateNodeState(nodedata);
        return true;
    }
    return false;
}

// note : all check is for secondary node (except for electing).
// It should be called only in the thread of event handler in ZooKeeper(like onDataChanged)
//  or be called while starting up and stopping. 
//  Any other call may cause deadlock.
void NodeManagerBase::checkPrimaryState(bool primary_deleted)
{
    if (stopping_)
        return;
    // 
    if (primary_deleted)
    {
        setElectingState();
        checkForPrimaryElecting();
        return;
    }
    NodeStateType primary_state = getPrimaryState();
    LOG(INFO) << "current primary node changed : " << primary_state;

    if (primary_state == NODE_STATE_UNKNOWN)
    {
        LOG(WARNING) << "primary may lost .";
        if (!zookeeper_->isZNodeExists(curr_primary_path_, ZooKeeper::WATCH))
        {
            setElectingState();
            checkForPrimaryElecting();
            return;
        }
    }

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
    case NODE_STATE_ELECTING:
        checkPrimaryForFinishElecting(primary_state);
    default:
        break;
    }
}

void NodeManagerBase::checkPrimaryForFinishElecting(NodeStateType primary_state)
{
    if (!s_enable_async_)
        return;
    if (primary_state != NODE_STATE_ELECTING && !need_check_electing_)
    {
        LOG(INFO) << "primary electing finished. replicas begin sync to new.";
        nodeState_ = NODE_STATE_STARTED;
        updateNodeState();
    }
}

void NodeManagerBase::checkPrimaryForStartNewWrite(NodeStateType primary_state)
{
    if (s_enable_async_ && !isPrimaryWithoutLock())
    {
        // in async mode, we just wake up the async log worker to pull new log from primary.
        DistributeTestSuit::testFail(ReplicaFail_At_ReqProcessing);
        cb_on_resume_sync_();
        return;
    }

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
            LOG(ERROR) << "may be lost from ZooKeeper, waiting reconnected";
            return;
            // primary is ok, this error can not handle. 
            //RecoveryChecker::forceExit("exit for unrecovery node state.");
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
    if (s_enable_async_)
    {
        checkForAsyncWrite();
        return;
    }
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
            updateSelfPrimaryNodeState();
    }
}

void NodeManagerBase::checkPrimaryForAbortWrite(NodeStateType primary_state)
{
    if (s_enable_async_)
    {
        LOG(ERROR) << "in async this should never happen." << nodeState_;
        return;
    }
    DistributeTestSuit::testFail(ReplicaFail_At_Waiting_Primary_Abort);
    if (primary_state == NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_ABORT ||
        primary_state == NODE_STATE_ELECTING)
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
        //nodeState_ = NODE_STATE_STARTED;
        enterClusterAfterRecovery(!masterStarted_);
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
    uint32_t newest_reqid = 0;
    for (size_t i = 0; i < node_list.size(); ++i)
    {
        NodeStateType state = getNodeState(node_list[i]);
        if (node_list[i] == curr_primary_path_)
            continue;
        if (s_enable_async_)
        {
            if (state != NODE_STATE_ELECTING)
            {
                all_secondary_ready = false;
                LOG(INFO) << "one secondary node not ready for electing : " <<
                    node_list[i] << ", state: " << state << ", keep waiting.";
                break;
            }
            std::string sdata;
            if (zookeeper_->getZNodeData(node_list[i], sdata, ZooKeeper::WATCH))
            {
                ZNode znode;
                znode.loadKvString(sdata);
                if (newest_reqid < znode.getUInt32Value(ZNode::KEY_LAST_WRITE_REQID))
                {
                    newest_reqid = znode.getUInt32Value(ZNode::KEY_LAST_WRITE_REQID);
                }
            }
            else
            {
                all_secondary_ready = false;
                break;
            }
        }
        else
        {
            if (state != NODE_STATE_STARTED)
            {
                all_secondary_ready = false;
                LOG(INFO) << "one secondary node not ready during electing: " <<
                    node_list[i] << ", state: " << state << ", keep waiting.";
                break;
            }
        }
    }

    if (node_list.size() == 0)
    {
        LOG(INFO) << "empty children!!!! ZooKeeper may lost" << nodePath_;
        all_secondary_ready = false;
    }

    if (all_secondary_ready)
    {
        if (s_enable_async_)
        {
            LOG(INFO) << "In async write mode, all secondary is ready for electing, current primary checking electing.";
            std::string sdata;
            uint32_t my_logid = 0;
            if (zookeeper_->getZNodeData(self_primary_path_, sdata, ZooKeeper::WATCH))
            {
                ZNode znode;
                znode.loadKvString(sdata);
                my_logid = znode.getUInt32Value(ZNode::KEY_LAST_WRITE_REQID);
            }

            LOG(INFO) << "my logid " << my_logid << ", newest logid : " << newest_reqid;
            if (my_logid < newest_reqid)
            {
                LOG(INFO) << "I am not the newest while electing, abandon myself primary, and re-enter.";
                unregisterPrimary();
                nodeState_ = NODE_STATE_STARTING;
                sleep(10);
                reEnterCluster();
                return;
            }
            LOG(INFO) << "current primary is newest, keep mine primary!";
        }
        LOG(INFO) << "all secondary ready, ending electing, primary is ready for new request: " << curr_primary_path_;
        if (cb_on_elect_finished_)
            cb_on_elect_finished_();
        MasterManagerBase::get()->notifyChangedPrimary();
        NodeStateType oldstate = nodeState_;
        updateNodeStateToNewState(NODE_STATE_STARTED);
        if (s_enable_async_ && oldstate != NODE_STATE_STARTED)
            updateNodeState();
    }
    else if(!self_changed)
    {
        updateNodeState();
    }
}

bool NodeManagerBase::isNeedCheckElecting()
{
    return need_check_electing_;
}

// for out caller
void NodeManagerBase::setNodeState(NodeStateType state)
{
    boost::unique_lock<boost::mutex> lock(mutex_);
    if (stopping_)
        return;
    //updateCurrentPrimary();
    if (isNeedCheckElecting())
    {
        LOG(INFO) << "try to change state to new while electing : " << state;
        if (!s_enable_async_)
        {
            nodeState_ = state;
            updateNodeState();
            MasterManagerBase::get()->disableNewWrite();
            return;
        }
    }
    updateNodeStateToNewState(state);
}

uint32_t NodeManagerBase::getLastWriteReqId()
{
    std::string sdata;
    ZNode znode;
    zookeeper_->getZNodeData(primaryNodeParentPath_, sdata, ZooKeeper::WATCH);
    if (!sdata.empty())
    {
        znode.loadKvString(sdata);
        return znode.getUInt32Value(ZNode::KEY_LAST_WRITE_REQID);
    }
    return 0;
}

void NodeManagerBase::updateLastWriteReqId(uint32_t req_id)
{
    // update the success request id to ZooKeeper
    // This will be used for checking whether request log is newest after auto-reconnected
    //
    LOG(INFO) << "update last success request for current replica set : " << req_id;
    if (s_enable_async_)
    {
        // in async mode, each node in replicas will update itself newest logid while 
        // primary is electing. The node with the newest logid will be elected as 
        // new primary, and other node need re-register and sync to the new primary.
        return;
    }
    uint32_t cur_req_id = getLastWriteReqId();
    if( req_id < cur_req_id)
    {
        LOG(ERROR) << "update the last write request id is smaller than the id on replica set parent node.";
        if (req_id != 0)
        {
            RecoveryChecker::forceExit("Update Last Write request id error.");
        }
        LOG(INFO) << "last request id is reseted to 0.";
    }
    else if (req_id == cur_req_id)
        return;
    ZNode znode;
    znode.setValue(ZNode::KEY_LAST_WRITE_REQID, req_id);
    zookeeper_->setZNodeData(primaryNodeParentPath_, znode.serialize());
}

void NodeManagerBase::updateNodeStateToNewState(NodeStateType new_state)
{
    if (new_state == nodeState_)
        return;
    NodeStateType oldstate = nodeState_;
    nodeState_ = new_state;
    ZNode nodedata;
    ZNode oldZnode;

    if (s_enable_async_ && nodeState_ != NODE_STATE_STARTED)
    {
        if (checkForAsyncWrite())
        {
            if (oldstate != NODE_STATE_STARTED)
                return;
        }
        else
        {
            LOG(INFO) << "need update self_primary_path_ : " << nodeState_;
            setSf1rNodeData(nodedata, oldZnode);
            zookeeper_->setZNodeData(self_primary_path_, nodedata.serialize());
        }
    }

    if (!s_enable_async_)
    {
        setSf1rNodeData(nodedata, oldZnode);
    }

    bool need_update = nodedata.getStrValue(ZNode::KEY_SERVICE_STATE) != oldZnode.getStrValue(ZNode::KEY_SERVICE_STATE) ||
                  nodedata.getStrValue(ZNode::KEY_SELF_REG_PRIMARY_PATH) != oldZnode.getStrValue(ZNode::KEY_SELF_REG_PRIMARY_PATH);

    if (nodeState_ == NODE_STATE_STARTED && !need_check_electing_)
    {
	    MasterManagerBase::get()->enableNewWrite();
        if (s_enable_async_ && !isPrimaryWithoutLock())
        {
            cb_on_resume_sync_();
        }
    }

    if (need_stop_ && nodeState_ == NODE_STATE_STARTED && !MasterManagerBase::get()->hasAnyCachedRequest())
    {
        stop();
        return;
    }
    if (!s_enable_async_)
    {
        zookeeper_->setZNodeData(self_primary_path_, nodedata.serialize());
    }
    // notify master got ready for next request.
    if (masterStarted_)
    {
        if(oldstate == NODE_STATE_STARTED)
        {
            LOG(INFO) << "notify master busy for current request ... ";
            MasterManagerBase::get()->updateMasterReadyForNew(false);
        }
        else if (new_state == NODE_STATE_STARTED)
        {
            LOG(INFO) << "notify master ready for new request ... ";
            MasterManagerBase::get()->updateMasterReadyForNew(true);
        }
        if (need_update)
        {
            LOG(INFO) << "update node path in topology for service state changed ... ";
            zookeeper_->setZNodeData(nodePath_, nodedata.serialize());
            MasterManagerBase::get()->updateServiceReadState(nodedata.getStrValue(ZNode::KEY_SERVICE_STATE), false);
        }
    }
}

void NodeManagerBase::setSlowWriting()
{
    boost::unique_lock<boost::mutex> lock(mutex_);
    if (stopping_)
        return;
    if (!zookeeper_ || !zookeeper_->isConnected())
        return;
    slow_write_running_ = true;
    updateNodeState();
}

void NodeManagerBase::updateSelfPrimaryNodeState()
{
    if (s_enable_async_)
    {
        if (checkForAsyncWrite())
        {
            return;
        }
    }
    ZNode nodedata;
    setSf1rNodeData(nodedata);
    if (s_enable_async_)
        LOG(INFO) << "update self_primary_path_ in async mode : " << nodeState_;
    updateSelfPrimaryNodeState(nodedata);
}

void NodeManagerBase::updateNodeState()
{
    ZNode nodedata;
    setSf1rNodeData(nodedata);
    updateNodeState(nodedata);
}

void NodeManagerBase::updateSelfPrimaryNodeState(const ZNode& nodedata)
{
    if (nodeState_ == NODE_STATE_STARTED && need_stop_ && !MasterManagerBase::get()->hasAnyCachedRequest())
    {
        stop();
    }
    else
    {
        zookeeper_->setZNodeData(self_primary_path_, nodedata.serialize());
    }
}

void NodeManagerBase::updateNodeState(const ZNode& nodedata)
{
    // update to nodepath to make master got notified.
    zookeeper_->setZNodeData(nodePath_, nodedata.serialize());
    if (masterStarted_)
    {
        MasterManagerBase::get()->updateServiceReadState(nodedata.getStrValue(ZNode::KEY_SERVICE_STATE), false);
        if (!need_check_electing_ && nodeState_ == NODE_STATE_STARTED)
        {
            MasterManagerBase::get()->enableNewWrite();
        }
    }
    if (!need_check_electing_ && nodeState_ == NODE_STATE_STARTED && s_enable_async_
        && !isPrimaryWithoutLock())
    {
        cb_on_resume_sync_();
    }

    updateSelfPrimaryNodeState(nodedata);
}

void NodeManagerBase::checkSecondaryReqProcess(bool self_changed)
{
    if (nodeState_ != NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_FINISH_PROCESS)
    {
        LOG(INFO) << "only in waiting replica state, check secondary request process need. state: " << nodeState_;
        return;
    }
    if (s_enable_async_)
    {
        checkForAsyncWrite();
        return;
    }

    bool all_secondary_ready = true;
    std::vector<std::string> node_list;
    zookeeper_->getZNodeChildren(primaryNodeParentPath_, node_list, ZooKeeper::WATCH);
    LOG(INFO) << "in request processing state found " << node_list.size() << " children in node " << primaryNodeParentPath_;
    for (size_t i = 0; i < node_list.size(); ++i)
    {
        NodeStateType state = getNodeState(node_list[i]);
        if (node_list[i] == curr_primary_path_)
            continue;
        if (state == NODE_STATE_PROCESSING_REQ_RUNNING || state == NODE_STATE_STARTED ||
            state == NODE_STATE_UNKNOWN )
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

    if (node_list.size() == 0)
    {
        LOG(INFO) << "empty children!!!! ZooKeeper may lost" << nodePath_;
        all_secondary_ready = false;
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
            updateSelfPrimaryNodeState();
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
    if (s_enable_async_)
    {
        checkForAsyncWrite();
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
        if (state == NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY ||
            state == NODE_STATE_UNKNOWN )
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

    if (node_list.size() == 0)
    {
        LOG(INFO) << "empty children!!!! ZooKeeper may lost" << nodePath_;
        all_secondary_ready = false;
    }

    if (all_secondary_ready)
    {
        // all replica finished write log
        // ready for next new request.
        if (cb_on_wait_finish_log_)
            cb_on_wait_finish_log_();
        LOG(INFO) << "all nodes finished write log.";
        if (need_check_recover_)
        {
            checkSecondaryRecovery(self_changed);
        }
        else
            updateNodeStateToNewState(NODE_STATE_STARTED);
    }
    else if (!self_changed)
    {
        updateSelfPrimaryNodeState();
    }
}

void NodeManagerBase::checkSecondaryReqAbort(bool self_changed)
{
    if (nodeState_ != NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_ABORT)
    {
        LOG(INFO) << "only in abort state, check secondary abort need. state: " << nodeState_;
        return;
    }
    if (s_enable_async_)
    {
        LOG(INFO) << "abortting request :" << self_primary_path_;
        if(cb_on_wait_replica_abort_)
            cb_on_wait_replica_abort_();
        updateNodeStateToNewState(NODE_STATE_STARTED);
        updateNodeState();
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
            state == NODE_STATE_PROCESSING_REQ_RUNNING ||
            state == NODE_STATE_UNKNOWN)
        {
            all_secondary_ready = false;
            LOG(INFO) << "one secondary node did not abort while waiting abort request: " <<
                node_list[i] << ", state: " << state << ", keep waiting.";
            break;
        }
    }

    if (node_list.size() == 0)
    {
        LOG(INFO) << "empty children!!!! ZooKeeper may lost" << nodePath_;
        all_secondary_ready = false;
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
        updateSelfPrimaryNodeState();
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
    bool can_recovery = false;
    if (is_any_recovery_waiting)
    {
        can_recovery = MasterManagerBase::get()->disableNewWrite();
        if (can_recovery)
        {
            if (!self_changed)
            {
                nodeState_ = NODE_STATE_RECOVER_WAIT_REPLICA_FINISH;
                updateSelfPrimaryNodeState();
            }
            else
                updateNodeStateToNewState(NODE_STATE_RECOVER_WAIT_REPLICA_FINISH);
            need_check_recover_ = false;
        }
    }
    else if (node_list.size() > 0)
    {
        need_check_recover_ = false;
    }
    if (!can_recovery)
    {
        NodeStateType new_state = NODE_STATE_STARTED;
        NodeStateType oldstate = nodeState_;
        if (cb_on_recover_wait_replica_finish_)
            cb_on_recover_wait_replica_finish_();
        updateNodeStateToNewState(new_state);
        if (s_enable_async_ && oldstate != NODE_STATE_STARTED)
        {
            updateNodeState();
        }
    }
}

// some state can handle without ZooKeeper if async write enabled.
// return true to indicate that event handled, return false means
// we should handle it by communicating with ZooKeeper.
bool NodeManagerBase::checkForAsyncWrite()
{
    switch(nodeState_)
    {
    case NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_FINISH_PROCESS:
        {
            if (cb_on_wait_finish_process_)
                cb_on_wait_finish_process_();
            if (cb_on_wait_finish_log_)
                cb_on_wait_finish_log_();
            LOG(INFO) << "finished write log.";
            if (need_check_recover_)
            {
                checkSecondaryRecovery(true);
            }
            else
                updateNodeStateToNewState(NODE_STATE_STARTED);
        }
        break;
    case NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_FINISH_LOG:
        {
            LOG(WARNING) << "In async write the nodeState_ is wrong : " << nodeState_;
            if (cb_on_wait_finish_log_)
                cb_on_wait_finish_log_();
            LOG(INFO) << "finished write log.";
            if (need_check_recover_)
            {
                checkSecondaryRecovery(true);
            }
            else
                updateNodeStateToNewState(NODE_STATE_STARTED);
        }
        break;
    case NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY:
        {
            if (cb_on_wait_primary_)
                cb_on_wait_primary_();
            LOG(INFO) << "write request log success on replica: " << self_primary_path_;
            updateNodeStateToNewState(NODE_STATE_STARTED);
        }
        break;
    case NODE_STATE_PROCESSING_REQ_RUNNING:
        LOG(INFO) << "write request running.";
        break;
    default:
        return false;
        break;
    }
    return true;
}

}
