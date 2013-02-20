#include "MasterManagerBase.h"
#include "SuperNodeManager.h"
#include "NodeManagerBase.h"
#include "ZooKeeperNamespace.h"

#include <boost/lexical_cast.hpp>

using namespace sf1r;

// note lock:
// the lock sequence is state_mutex_ and then workers_mutex_
// so you should never lock state_mutex_ after the workers_mutex_ is locked.
//
MasterManagerBase::MasterManagerBase()
: isDistributeEnable_(false)
, masterState_(MASTER_STATE_INIT)
, CLASSNAME("MasterManagerBase")
{
}

bool MasterManagerBase::init()
{
    // initialize zookeeper client
    topologyPath_ = ZooKeeperNamespace::getTopologyPath();
    serverParentPath_ = ZooKeeperNamespace::getServerParentPath();
    serverPath_ = ZooKeeperNamespace::getServerPath();
    zookeeper_ = ZooKeeperManager::get()->createClient(this);

    if (!zookeeper_)
        return false;

    sf1rTopology_ = NodeManagerBase::get()->getSf1rTopology();
    write_req_queue_ = ZooKeeperNamespace::getWriteReqQueueNode();
    write_req_queue_parent_ = ZooKeeperNamespace::getWriteReqQueueParent();
    return true;
}

void MasterManagerBase::start()
{
    boost::lock_guard<boost::mutex> lock(state_mutex_);
    if (masterState_ == MASTER_STATE_INIT)
    {
        masterState_ = MASTER_STATE_STARTING;

        if (!init())
        {
            throw std::runtime_error(std::string("failed to initialize ") + CLASSNAME);
        }

        if (!checkZooKeeperService())
        {
            masterState_ = MASTER_STATE_STARTING_WAIT_ZOOKEEPER;
            LOG (ERROR) << CLASSNAME << " waiting for ZooKeeper Service...";
            return;
        }

        doStart();
    }
    // call init for all service.
    ServiceMapT::const_iterator cit = all_distributed_services_.begin();
    while(cit != all_distributed_services_.end())
    {
        cit->second->initMaster();
        ++cit;
    }

}

void MasterManagerBase::stop()
{
    // disconnect will wait other ZooKeeper event finished,
    // so we can not do it in state_mutex_ lock.
    zookeeper_->disconnect();
    boost::lock_guard<boost::mutex> lock(state_mutex_);
    masterState_ = MASTER_STATE_INIT;
}

bool MasterManagerBase::getShardReceiver(
        unsigned int shardid,
        std::string& host,
        unsigned int& recvPort)
{
    boost::lock_guard<boost::mutex> lock(workers_mutex_);

    WorkerMapT::iterator it = workerMap_.find(shardid);
    if (it != workerMap_.end())
    {
        host = it->second->host_;
        recvPort = it->second->dataPort_;
        return true;
    }
    else
    {
        return false;
    }
}

bool MasterManagerBase::getCollectionShardids(const std::string& service, const std::string& collection, std::vector<shardid_t>& shardidList)
{
    boost::lock_guard<boost::mutex> lock(workers_mutex_);
    return sf1rTopology_.curNode_.master_.getShardidList(service, collection, shardidList);
}

bool MasterManagerBase::checkCollectionShardid(const std::string& service, const std::string& collection, unsigned int shardid)
{
    boost::lock_guard<boost::mutex> lock(workers_mutex_);
    return sf1rTopology_.curNode_.master_.checkCollectionWorker(service, collection, shardid);
}

void MasterManagerBase::registerIndexStatus(const std::string& collection, bool isIndexing)
{
    std::string indexStatus = isIndexing ? "indexing" : "notindexing";

    std::string data;
    if (zookeeper_ && zookeeper_->getZNodeData(serverRealPath_, data))
    {
        ZNode znode;
        znode.loadKvString(data);
        znode.setValue(collection, indexStatus);
    }

    std::string nodePath = getNodePath(sf1rTopology_.curNode_.replicaId_,  sf1rTopology_.curNode_.nodeId_);
    if (zookeeper_ && zookeeper_->getZNodeData(nodePath, data, ZooKeeper::WATCH))
    {
        ZNode znode;
        znode.loadKvString(data);
        znode.setValue(collection, indexStatus);
    }

}

void MasterManagerBase::process(ZooKeeperEvent& zkEvent)
{
    LOG(INFO) << CLASSNAME << ", "<< state2string(masterState_) <<", "<<zkEvent.toString();

    if (zkEvent.type_ == ZOO_SESSION_EVENT && zkEvent.state_ == ZOO_CONNECTED_STATE)
    {
        boost::lock_guard<boost::mutex> lock(state_mutex_);
        if (masterState_ == MASTER_STATE_STARTING_WAIT_ZOOKEEPER)
        {
            masterState_ = MASTER_STATE_STARTING;
            doStart();
        }
    }
    else if (zkEvent.type_ == ZOO_SESSION_EVENT && zkEvent.state_ == ZOO_EXPIRED_SESSION_STATE)
    {
        boost::lock_guard<boost::mutex> lock(state_mutex_);
        LOG(WARNING) << "master node disconnected by zookeeper, state : " << zookeeper_->getStateString();
        LOG(WARNING) << "try reconnect: " << sf1rTopology_.curNode_.toString();
         
        zookeeper_->disconnect();
        masterState_ = MASTER_STATE_STARTING;
        if (!checkZooKeeperService())
        {
            masterState_ = MASTER_STATE_STARTING_WAIT_ZOOKEEPER;
            LOG (ERROR) << CLASSNAME << " waiting for ZooKeeper Service...";
            return;
        }
        doStart();
        LOG (WARNING) << " restarted in MasterManagerBase for ZooKeeper Service finished";
    }
}

void MasterManagerBase::onNodeCreated(const std::string& path)
{
    boost::lock_guard<boost::mutex> lock(state_mutex_);

    if (masterState_ == MASTER_STATE_STARTING_WAIT_WORKERS)
    {
        // try detect workers
        masterState_ = MASTER_STATE_STARTING;
        detectWorkers();
    }
    else if (masterState_ == MASTER_STATE_STARTED
             && masterState_ != MASTER_STATE_RECOVERING)
    {
        // try recover
        recover(path);
    }
    //else if (masterState_ == MASTER_STATE_STARTED
    //             && masterState_ != MASTER_STATE_FAILOVERING)
    //{
    //    // try recover
    //    failover(path);
    //}
}

void MasterManagerBase::onNodeDeleted(const std::string& path)
{
    LOG(INFO) << "node deleted: " << path;
    boost::lock_guard<boost::mutex> lock(state_mutex_);

    if (masterState_ == MASTER_STATE_STARTED)
    {
        // try failover
        failover(path);
    }
    checkForWriteReq();
}

void MasterManagerBase::onChildrenChanged(const std::string& path)
{
    LOG(INFO) << "node children changed : " << path;
    boost::lock_guard<boost::mutex> lock(state_mutex_);

    if (masterState_ > MASTER_STATE_STARTING_WAIT_ZOOKEEPER)
    {
        detectReplicaSet(path);
    }
    checkForWriteReq();
}

void MasterManagerBase::onDataChanged(const std::string& path)
{
    LOG(INFO) << "node data changed : " << path;
    boost::lock_guard<boost::mutex> lock(state_mutex_);
    checkForWriteReq();
}

bool MasterManagerBase::prepareWriteReq()
{
    if (!isDistributeEnable_)
        return true;
    if (!zookeeper_ || !zookeeper_->isConnected())
        return false;
    if (!isMinePrimary())
    {
        LOG(WARNING) << "non-primary master can not prepare a write request!";
        return false;
    }
    ZNode znode;
    znode.setValue(ZNode::KEY_MASTER_SERVER_REAL_PATH, serverRealPath_);
    //znode.setValue(ZNode::KEY_MASTER_STATE, MASTER_STATE_WAIT_WORKER_FINISH_REQ);
    if (!zookeeper_->createZNode(ZooKeeperNamespace::getWriteReqNode(), znode.serialize(), ZooKeeper::ZNODE_EPHEMERAL))
    {
        if (zookeeper_->getErrorCode() == ZooKeeper::ZERR_ZNODEEXISTS)
        {
            LOG(INFO) << "There is another write request running, prepareWriteReq failed on server: " << serverRealPath_;
        }
        else
        {
            LOG (ERROR) <<" Failed to prepare write request for (" << zookeeper_->getErrorString()
                << "), please retry. on server : " << serverRealPath_;
        }
        return false;
    }
    LOG(INFO) << "prepareWriteReq success on server : " << serverRealPath_;
    //masterState_ = MASTER_STATE_WAIT_WORKER_FINISH_REQ;
    return true;
}

bool MasterManagerBase::getWriteReqNodeData(ZNode& znode)
{
    if (!zookeeper_->isZNodeExists(ZooKeeperNamespace::getWriteReqNode(), ZooKeeper::WATCH))
    {
        LOG(INFO) << "There is no any write request";
        return true;
    }
    std::string sdata;
    if (zookeeper_->getZNodeData(ZooKeeperNamespace::getWriteReqNode(), sdata, ZooKeeper::WATCH))
    {
        ZNode znode;
        znode.loadKvString(sdata);
    }
    else
    {
        LOG(WARNING) << "get write request data failed on :" << serverRealPath_;
    }
    return false;
}

void MasterManagerBase::checkForWriteReq()
{
    if (!isDistributeEnable_)
        return;
    if (!zookeeper_ || !zookeeper_->isConnected())
        return;

    if (!isMinePrimary())
    {
        LOG(INFO) << "not a primary master while check write request, ignore." << serverRealPath_;
        return;
    }

    switch(masterState_)
    {
    //case MASTER_STATE_WAIT_WORKER_FINISH_REQ:
    //    checkForWriteReqFinished();
    //    break;
    case MASTER_STATE_STARTED:
    case MASTER_STATE_STARTING_WAIT_WORKERS:
        checkForNewWriteReq();
        break;
    default:
        break;
    }
}

// check if last request finished
//void MasterManagerBase::checkForWriteReqFinished()
//{
//    if (masterState_ != MASTER_STATE_WAIT_WORKER_FINISH_REQ)
//    {
//        LOG(ERROR) << "master is not waiting worker finish request while check finish, state:" << masterState_;
//        return;
//    }
//    if (!isAllWorkerFinished())
//    {
//        LOG(INFO) << "not all worker finished current request, keep waiting.";
//        return;
//    }
//    masterState_ = MASTER_STATE_STARTED;
//    // update write request state to notify all primary worker.
//    ZNode znode;
//    if (getWriteReqNodeData(znode))
//    {
//        std::string write_server = znode.getStrValue(ZNode::KEY_MASTER_SERVER_REAL_PATH);
//        if (write_server != serverRealPath_)
//        {
//            LOG(WARNING) << "change write request state mismatch server. " << write_server << " vs " << serverRealPath_;
//            return;
//        }
//        znode.setValue(ZNode::KEY_MASTER_STATE, masterState_);
//        zookeeper_->setZNodeData(ZooKeeperNamespace::getWriteReqNode(), znode.serialize());
//        LOG(INFO) << "write request state changed success on server : " << serverRealPath_;
//    }
//}

// check if any new request can be processed.
void MasterManagerBase::checkForNewWriteReq()
{
    if (masterState_ != MASTER_STATE_STARTED || masterState_ == MASTER_STATE_STARTING_WAIT_WORKERS)
    {
        LOG(INFO) << "current master state is not ready while check write, state:" << masterState_;
        return;
    }
    if (!isAllWorkerIdle())
    {
        return;
    }
    if (!endWriteReq())
        return;
    std::vector<std::string> reqchild;
    zookeeper_->getZNodeChildren(write_req_queue_parent_, reqchild, ZooKeeper::WATCH);
    if (!reqchild.empty())
    {
        LOG(INFO) << "there are some write request waiting: " << reqchild.size();
        if (on_new_req_available_)
        {
            bool ret = on_new_req_available_();
            if (!ret)
            {
                LOG(INFO) << "the write request handler return failed.";
            }
            else
            {
                LOG(INFO) << "all new write requests have been delivered success.";
            }
        }
        else
        {
            LOG(ERROR) << "the new request handler not set!!";
            return;
        }
    }
}

// make sure prepare success before call this.
bool MasterManagerBase::popWriteReq(std::string& reqdata, std::string& type)
{
    if (!isDistributeEnable_)
        return false;
    if (!zookeeper_ || !zookeeper_->isConnected())
        return false;
    std::vector<std::string> reqchild;
    zookeeper_->getZNodeChildren(write_req_queue_parent_, reqchild);
    if (reqchild.empty())
    {
        LOG(INFO) << "no write request anymore while pop request on server: " << serverRealPath_;
        zookeeper_->getZNodeChildren(write_req_queue_parent_, reqchild, ZooKeeper::WATCH);
        return false;
    }
    ZNode znode;
    std::string sdata;
    zookeeper_->getZNodeData(reqchild[0], sdata);
    znode.loadKvString(sdata);
    reqdata = znode.getStrValue(ZNode::KEY_REQ_DATA);
    type = znode.getStrValue(ZNode::KEY_REQ_TYPE);
    LOG(INFO) << "a request poped : " << reqchild[0] << " on the server: " << serverRealPath_;
    zookeeper_->deleteZNode(reqchild[0]);
    return true;
}

void MasterManagerBase::pushWriteReq(const std::string& reqdata, const std::string& type)
{
    if (!isDistributeEnable_)
    {
        LOG(ERROR) << "Master is not configured as distributed, write request pushed failed." <<
            "," << reqdata;
        return;
    }
    boost::lock_guard<boost::mutex> lock(state_mutex_);
    if (!zookeeper_ || !zookeeper_->isConnected())
    {
        LOG(ERROR) << "Master is not connecting to ZooKeeper, write request pushed failed." <<
            "," << reqdata;
        return;
    }
    ZNode znode;
    //znode.setValue(ZNode::KEY_REQ_CONTROLLER, controller_name);
    znode.setValue(ZNode::KEY_REQ_TYPE, type);
    znode.setValue(ZNode::KEY_REQ_DATA, reqdata);
    if(zookeeper_->createZNode(write_req_queue_, znode.serialize(), ZooKeeper::ZNODE_SEQUENCE))
    {
        LOG(INFO) << "a write request pushed to the queue : " << zookeeper_->getLastCreatedNodePath();
    }
    else
    {
        LOG(ERROR) << "write request pushed failed." <<
            "," << reqdata;
    }
}

bool MasterManagerBase::endWriteReq()
{
    if (!isMinePrimary())
    {
        LOG(INFO) << "non-primary master can not end a write request.";
        return false;
    }
    if (!zookeeper_->isZNodeExists(ZooKeeperNamespace::getWriteReqNode(), ZooKeeper::WATCH))
    {
        LOG(INFO) << "There is no any write request while end request";
        return true;
    }
    std::string sdata;
    if (zookeeper_->getZNodeData(ZooKeeperNamespace::getWriteReqNode(), sdata, ZooKeeper::WATCH))
    {
        ZNode znode;
        znode.loadKvString(sdata);
        std::string write_server = znode.getStrValue(ZNode::KEY_MASTER_SERVER_REAL_PATH);
        if (write_server != serverRealPath_)
        {
            LOG(WARNING) << "end request mismatch server. " << write_server << " vs " << serverRealPath_;
            return false;
        }
        zookeeper_->deleteZNode(ZooKeeperNamespace::getWriteReqNode());
        LOG(INFO) << "end write request success on server : " << serverRealPath_;
    }
    else
    {
        LOG(WARNING) << "get write request data failed while end request on server :" << serverRealPath_;
        return false;
    }
    return true;
}

//bool MasterManagerBase::isAllWorkerFinished()
//{
//    if (!isAllWorkerInState(NodeManagerBase::NODE_STATE_WAIT_MASTER_FINISH_REQ))
//    {
//        LOG(INFO) << "one of primary worker not finish write request : ";
//        return false;
//    }
//    return true;
//}

bool MasterManagerBase::isAllWorkerIdle()
{
    if (!isAllWorkerInState(NodeManagerBase::NODE_STATE_STARTED))
    {
        LOG(INFO) << "one of primary worker not ready for new write request.";
        return false;
    }
    return true;
}

bool MasterManagerBase::isAllWorkerInState(int state)
{
    boost::lock_guard<boost::mutex> lock(workers_mutex_);
    WorkerMapT::iterator it;
    for (it = workerMap_.begin(); it != workerMap_.end(); it++)
    {
        std::string nodepath = getNodePath(it->second->replicaId_,  it->first);
        std::string sdata;
        if (zookeeper_->getZNodeData(nodepath, sdata, ZooKeeper::WATCH))
        {
            ZNode nodedata;
            nodedata.loadKvString(sdata);
            if (nodedata.getUInt32Value(ZNode::KEY_NODE_STATE) != (uint32_t)state)
            {
                return false;
            }
        }
    }
    return true;
}

bool MasterManagerBase::isBusy()
{
    if (!isDistributeEnable_)
        return false;
    if (!zookeeper_ || !zookeeper_->isConnected())
        return true;
    if (zookeeper_->isZNodeExists(ZooKeeperNamespace::getWriteReqNode(), ZooKeeper::WATCH))
    {
        LOG(INFO) << "Master is busy because there is another write request running";
        return true;
    }
    return !isAllWorkerIdle();
}

void MasterManagerBase::showWorkers()
{
    WorkerMapT::iterator it;
    for (it = workerMap_.begin(); it != workerMap_.end(); it++)
    {
        cout << it->second->toString() ;
    }
}

/// protected ////////////////////////////////////////////////////////////////////

std::string MasterManagerBase::state2string(MasterStateType e)
{
    std::stringstream ss;
    switch (e)
    {
    case MASTER_STATE_INIT:
        return "MASTER_STATE_INIT";
        break;
    case MASTER_STATE_STARTING:
        return "MASTER_STATE_STARTING";
        break;
    case MASTER_STATE_STARTING_WAIT_ZOOKEEPER:
        return "MASTER_STATE_STARTING_WAIT_ZOOKEEPER";
        break;
    case MASTER_STATE_STARTING_WAIT_WORKERS:
        return "MASTER_STATE_STARTING_WAIT_WORKERS";
        break;
    case MASTER_STATE_STARTED:
        return "MASTER_STATE_STARTED";
        break;
    case MASTER_STATE_FAILOVERING:
        return "MASTER_STATE_FAILOVERING";
        break;
    case MASTER_STATE_RECOVERING:
        return "MASTER_STATE_RECOVERING";
        break;
    }

    return "UNKNOWN";
}

void MasterManagerBase::watchAll()
{
    // for replica change
    std::vector<std::string> childrenList;
    zookeeper_->getZNodeChildren(topologyPath_, childrenList, ZooKeeper::WATCH);
    for (size_t i = 0; i < childrenList.size(); i++)
    {
        std::vector<std::string> chchList;
        zookeeper_->getZNodeChildren(childrenList[i], chchList, ZooKeeper::WATCH);
    }

    // for nodes change
    for (uint32_t nodeid = 1; nodeid <= sf1rTopology_.nodeNum_; nodeid++)
    {
        std::string nodePath = getNodePath(sf1rTopology_.curNode_.replicaId_, nodeid);
        zookeeper_->isZNodeExists(nodePath, ZooKeeper::WATCH);
    }
}

bool MasterManagerBase::checkZooKeeperService()
{
    if (!zookeeper_->isConnected())
    {
        zookeeper_->connect(true);

        if (!zookeeper_->isConnected())
        {
            return false;
        }
    }

    return true;
}

void MasterManagerBase::doStart()
{
    detectReplicaSet();

    detectWorkers();

    // Each Master serves as a Search Server, register it without waiting for all workers to be ready.
    registerServiceServer();
}

int MasterManagerBase::detectWorkersInReplica(replicaid_t replicaId, size_t& detected, size_t& good)
{
    bool mine_primary = isMinePrimary();
    if (mine_primary)
        LOG(INFO) << "I am primary master ";

    for (uint32_t nodeid = 1; nodeid <= sf1rTopology_.nodeNum_; nodeid++)
    {
        std::string data;
        std::string nodePath = getNodePath(replicaId, nodeid);
        if (zookeeper_->getZNodeData(nodePath, data, ZooKeeper::WATCH))
        {
            ZNode znode;
            znode.loadKvString(data);

            // if this sf1r node provides worker server
            if (znode.hasKey(ZNode::KEY_WORKER_PORT))
            {
                if (mine_primary)
                {
                    if(!isPrimaryWorker(replicaId, nodeid))
                    {
                        LOG(INFO) << "primary master need detect primary worker, ignore non-primary worker";
                        LOG (INFO) << "node " << nodeid << ", replica: " << replicaId;
                        continue;
                    }
                }

                if (nodeid > 0 && nodeid <= sf1rTopology_.nodeNum_)
                {
                    if (workerMap_.find(nodeid) != workerMap_.end())
                    {
                        workerMap_[nodeid]->worker_.isGood_ = true;
                    }
                    else
                    {
                        // insert new worker
                        boost::shared_ptr<Sf1rNode> sf1rNode(new Sf1rNode);
                        sf1rNode->worker_.isGood_ = true;
                        workerMap_[nodeid] = sf1rNode;
                    }

                    // update worker info
                    boost::shared_ptr<Sf1rNode>& workerNode = workerMap_[nodeid];
                    workerNode->nodeId_ = nodeid;
                    updateWorkerNode(workerNode, znode);

                    detected ++;
                    if (workerNode->worker_.isGood_)
                    {
                        good ++;
                    }
                }
                else
                {
                    std::stringstream ss;
                    ss << "in node[" << nodeid << "] @ " << znode.getStrValue(ZNode::KEY_HOST)
                       << " is out of range for current master (max is " << sf1rTopology_.nodeNum_ << ")"; 

                    LOG (WARNING) << ss.str();
                    throw std::runtime_error(ss.str());
                }
            }
        }
        else
        {
            // reset watcher
            zookeeper_->isZNodeExists(nodePath, ZooKeeper::WATCH);
        }
    }

    if (detected >= sf1rTopology_.nodeNum_)
    {
        masterState_ = MASTER_STATE_STARTED;
        LOG (INFO) << CLASSNAME << " detected " << sf1rTopology_.nodeNum_
                   << " all workers (good " << good << ")" << std::endl;
    }
    else
    {
        masterState_ = MASTER_STATE_STARTING_WAIT_WORKERS;
        LOG (INFO) << CLASSNAME << " detected " << detected << " workers (good " << good
                   << "), all " << sf1rTopology_.nodeNum_ << std::endl;
    }
    return good;
}

int MasterManagerBase::detectWorkers()
{
    boost::lock_guard<boost::mutex> lock(workers_mutex_);

    size_t detected = 0;
    size_t good = 0;
    // detect workers from "current" replica first
    replicaid_t replicaId = sf1rTopology_.curNode_.replicaId_;
    detectWorkersInReplica(replicaId, detected, good);

    for (size_t i = 0; i < replicaIdList_.size(); i++)
    {
        if (masterState_ != MASTER_STATE_STARTING_WAIT_WORKERS)
        {
            LOG(INFO) << "detected worker enough, stop detect other replica.";
            break;
        }
        if (replicaId == replicaIdList_[i])
            continue;
        // not enough, check other replica
        LOG(INFO) << "begin detect workers in other replica : " << replicaIdList_[i];
        detectWorkersInReplica(replicaIdList_[i], detected, good);
    }
    //
    // update workers' info to aggregators
    resetAggregatorConfig();

    return good;
}

void MasterManagerBase::updateWorkerNode(boost::shared_ptr<Sf1rNode>& workerNode, ZNode& znode)
{
    workerNode->replicaId_ = sf1rTopology_.curNode_.replicaId_;
    workerNode->host_ = znode.getStrValue(ZNode::KEY_HOST);
    //workerNode->port_ = znode.getUInt32Value(ZNode::KEY_WORKER_PORT);

    try
    {
        workerNode->worker_.port_ =
                boost::lexical_cast<port_t>(znode.getStrValue(ZNode::KEY_WORKER_PORT));
    }
    catch (std::exception& e)
    {
        workerNode->worker_.isGood_ = false;
        LOG (ERROR) << "failed to convert workerPort \"" << znode.getStrValue(ZNode::KEY_WORKER_PORT)
                    << "\" got from worker on node " << workerNode->nodeId_
                    << " @" << workerNode->host_;
    }

    try
    {
        workerNode->dataPort_ =
                boost::lexical_cast<port_t>(znode.getStrValue(ZNode::KEY_DATA_PORT));
    }
    catch (std::exception& e)
    {
        workerNode->worker_.isGood_ = false;
        LOG (ERROR) << "failed to convert dataPort \"" << znode.getStrValue(ZNode::KEY_DATA_PORT)
                    << "\" got from worker on node " << workerNode->nodeId_
                    << " @" << workerNode->host_;
    }

    LOG (INFO) << CLASSNAME << " detected worker on (node" << workerNode->nodeId_ <<") "
               << workerNode->host_ << ":" << workerNode->worker_.port_ << std::endl;
}

void MasterManagerBase::detectReplicaSet(const std::string& zpath)
{
    boost::lock_guard<boost::mutex> lock(replica_mutex_);

    // find replications
    std::vector<std::string> childrenList;
    zookeeper_->getZNodeChildren(topologyPath_, childrenList, ZooKeeper::WATCH);

    replicaIdList_.clear();
    for (size_t i = 0; i < childrenList.size(); i++)
    {
        std::string sreplicaId;
        zookeeper_->getZNodeData(childrenList[i], sreplicaId);
        try
        {
            replicaIdList_.push_back(boost::lexical_cast<replicaid_t>(sreplicaId));
            LOG (INFO) << " detected replica id \"" << sreplicaId
                        << "\" for " << childrenList[i];
        }
        catch (std::exception& e) {
            LOG (ERROR) << CLASSNAME << " failed to parse replica id \"" << sreplicaId
                        << "\" for " << childrenList[i];
        }

        // watch for nodes change
        std::vector<std::string> chchList;
        zookeeper_->getZNodeChildren(childrenList[i], chchList, ZooKeeper::WATCH);
        zookeeper_->isZNodeExists(childrenList[i], ZooKeeper::WATCH);
    }

    // try to detect workers again while waiting for some of the workers
    if (masterState_ == MASTER_STATE_STARTING_WAIT_WORKERS)
    {
        detectWorkers();
    }

    WorkerMapT::iterator it;
    for (it = workerMap_.begin(); it != workerMap_.end(); it++)
    {
        boost::shared_ptr<Sf1rNode>& sf1rNode = it->second;
        if (!sf1rNode->worker_.isGood_)
        {
            // try failover
            failover(sf1rNode);
        }
    }
}

void MasterManagerBase::failover(const std::string& zpath)
{
    masterState_ = MASTER_STATE_FAILOVERING;

    // check path
    WorkerMapT::iterator it;
    for (it = workerMap_.begin(); it != workerMap_.end(); it++)
    {
        boost::shared_ptr<Sf1rNode>& sf1rNode = it->second;
        std::string nodePath = getNodePath(sf1rNode->replicaId_, sf1rNode->nodeId_);
        if (zpath == nodePath)
        {
            LOG (WARNING) << "[node " << sf1rNode->nodeId_ << "]@" << sf1rNode->host_ << " was broken down, in "
                          << "[replica " << sf1rNode->replicaId_ << "]";
            if (failover(sf1rNode))
            {
                LOG (INFO) << "failover: finished.";
            }
            else
            {
                LOG (INFO) << "failover: failed to cover this failure.";
            }
        }
    }

    masterState_ = MASTER_STATE_STARTED;
}

bool MasterManagerBase::failover(boost::shared_ptr<Sf1rNode>& sf1rNode)
{
    sf1rNode->worker_.isGood_ = false;
    bool mine_primary = isMinePrimary();
    if (mine_primary)
        LOG(INFO) << "I am primary master ";
    for (size_t i = 0; i < replicaIdList_.size(); i++)
    {
        if (replicaIdList_[i] != sf1rNode->replicaId_)
        {
            // try to switch to replicaIdList_[i]
            ZNode znode;
            std::string sdata;
            std::string nodePath = getNodePath(replicaIdList_[i], sf1rNode->nodeId_);
            // get node data
            if (zookeeper_->getZNodeData(nodePath, sdata, ZooKeeper::WATCH))
            {
                if (mine_primary)
                {
                    if(!isPrimaryWorker(replicaIdList_[i], sf1rNode->nodeId_))
                    {
                        LOG(INFO) << "primary master need failover to primary worker, ignore non-primary worker";
                        LOG (INFO) << "node " << sf1rNode->nodeId_ << " ,replica: " << replicaIdList_[i];
                        continue;
                    }
                }
                znode.loadKvString(sdata);
                if (znode.hasKey(ZNode::KEY_WORKER_PORT))
                {
                    LOG (INFO) << "switching node " << sf1rNode->nodeId_ << " from replica " << sf1rNode->replicaId_
                               <<" to " << replicaIdList_[i];

                    sf1rNode->replicaId_ = replicaIdList_[i]; // new replica
                    sf1rNode->host_ = znode.getStrValue(ZNode::KEY_HOST);
                    try
                    {
                        sf1rNode->worker_.port_ =
                                boost::lexical_cast<port_t>(znode.getStrValue(ZNode::KEY_WORKER_PORT));
                    }
                    catch (std::exception& e)
                    {
                        LOG (ERROR) << "failed to convert workerPort \"" << znode.getStrValue(ZNode::KEY_WORKER_PORT)
                                    << "\" got from node " << sf1rNode->nodeId_ << " at " << znode.getStrValue(ZNode::KEY_HOST)
                                    << ", in replica " << replicaIdList_[i];
                        continue;
                    }

                    // succeed to failover
                    sf1rNode->worker_.isGood_ = true;
                    break;
                }
                else
                {
                    LOG (ERROR) << "[Replica " << replicaIdList_[i] << "] [Node " << sf1rNode->nodeId_
                                << "] did not enable worker server, this happened because of the mismatch configuration.";
                    LOG (ERROR) << "In the same cluster, the sf1r node with the same nodeid must have the same configuration.";
                    throw std::runtime_error("error configuration : mismatch with the same nodeid.");
                }
            }
        }
    }

    // notify aggregators
    resetAggregatorConfig();

    // Watch current replica, waiting for node recover
    zookeeper_->isZNodeExists(getNodePath(sf1rNode->replicaId_, sf1rNode->nodeId_), ZooKeeper::WATCH);

    return sf1rNode->worker_.isGood_;
}


void MasterManagerBase::recover(const std::string& zpath)
{
    masterState_ = MASTER_STATE_RECOVERING;

    WorkerMapT::iterator it;
    bool mine_primary = isMinePrimary();
    if (mine_primary)
        LOG(INFO) << "I am primary master ";

    for (it = workerMap_.begin(); it != workerMap_.end(); it++)
    {
        boost::shared_ptr<Sf1rNode>& sf1rNode = it->second;
        if (zpath == getNodePath(sf1rTopology_.curNode_.replicaId_, sf1rNode->nodeId_))
        {
            if (mine_primary)
            {
                if(!isPrimaryWorker(sf1rTopology_.curNode_.replicaId_, sf1rNode->nodeId_))
                {
                    LOG(INFO) << "primary master need recover to primary worker, ignore non-primary worker";
                    LOG (INFO) << "node " << sf1rNode->nodeId_ << " ,replica: " << sf1rTopology_.curNode_.replicaId_;
                    continue;
                }
            }

            LOG (INFO) << "recover: node " << sf1rNode->nodeId_
                       << " recovered in current replica " << sf1rTopology_.curNode_.replicaId_;

            ZNode znode;
            std::string sdata;
            if (zookeeper_->getZNodeData(zpath, sdata, ZooKeeper::WATCH))
            {
                // try to recover
                znode.loadKvString(sdata);
                sf1rNode->replicaId_ = sf1rTopology_.curNode_.replicaId_; // new replica
                sf1rNode->host_ = znode.getStrValue(ZNode::KEY_HOST);
                try
                {
                    sf1rNode->worker_.port_ =
                            boost::lexical_cast<port_t>(znode.getStrValue(ZNode::KEY_WORKER_PORT));
                }
                catch (std::exception& e)
                {
                    LOG (ERROR) << "failed to convert workerPort \"" << znode.getStrValue(ZNode::KEY_WORKER_PORT)
                                << "\" got from node " << sf1rNode->nodeId_ << " at " << znode.getStrValue(ZNode::KEY_HOST)
                                << ", in replica " << sf1rTopology_.curNode_.replicaId_;
                    continue;
                }

                // recovered, and notify aggregators
                sf1rNode->worker_.isGood_ = true;
                break;
            }
        }
    }

    resetAggregatorConfig();
    masterState_ = MASTER_STATE_STARTED;
}

void MasterManagerBase::setServicesData(ZNode& znode)
{
    // write service name to server node.
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

void MasterManagerBase::initServices()
{
}

bool MasterManagerBase::isServiceReadyForRead(bool include_self)
{
    // service is ready for read means all shard workers current master connected are ready for read.
    boost::lock_guard<boost::mutex> lock(workers_mutex_);

    WorkerMapT::const_iterator it = workerMap_.begin();
    for ( ; it != workerMap_.end(); ++it)
    {
        if (it->second->nodeId_ == sf1rTopology_.curNode_.nodeId_)
        {
            if (!include_self)
                continue;
        }
        std::string nodepath = getNodePath(it->second->replicaId_, it->second->nodeId_);
        std::string sdata;
        if (zookeeper_->getZNodeData(nodepath, sdata, ZooKeeper::WATCH))
        {
            ZNode znode;
            znode.loadKvString(sdata);
            std::string value = znode.getStrValue(ZNode::KEY_SERVICE_STATE);
            if (value != "ReadyForRead")
            {
                LOG(INFO) << "one shard of master service is not ready for read:" << nodepath;
                return false;
            }
        }
    }
    return true;
}

void MasterManagerBase::registerDistributeServiceMaster(boost::shared_ptr<IDistributeService> sp_service, bool enable_master)
{
    if (enable_master)
    {
        if (all_distributed_services_.find(sp_service->getServiceName()) != 
            all_distributed_services_.end() )
        {
            LOG(WARNING) << "duplicate service name!!!!!!!";
            throw std::runtime_error("duplicate service!");
        }
        LOG(INFO) << "registering service master: " << sp_service->getServiceName();
        all_distributed_services_[sp_service->getServiceName()] = sp_service;
    }
}

bool MasterManagerBase::findServiceMasterAddress(const std::string& service, std::string& host, uint32_t& port)
{
    if (!zookeeper_ || !zookeeper_->isConnected())
        return false;

    std::vector<std::string> children;
    zookeeper_->getZNodeChildren(serverParentPath_, children);

    for (size_t i = 0; i < children.size(); ++i)
    {
        std::string serviceMasterPath = children[i];
        if (!zookeeper_->isZNodeExists(serviceMasterPath + "/" + service))
        {
            // no this service on this master server.
            continue;
        }

        LOG(INFO) << "find service master address success : " << service << ", on server :" << serviceMasterPath;
        std::string data;
        if (zookeeper_->getZNodeData(serviceMasterPath, data))
        {
            ZNode znode;
            znode.loadKvString(data);

            host = znode.getStrValue(ZNode::KEY_HOST);
            port = znode.getUInt32Value(ZNode::KEY_MASTER_PORT);
            return true;
        }
    }
    return false;
}

void MasterManagerBase::registerServiceServer()
{
    // Master server provide search service
    if (!zookeeper_->isZNodeExists(serverParentPath_))
    {
        zookeeper_->createZNode(serverParentPath_);
    }

    initServices();

    ZNode znode;
    znode.setValue(ZNode::KEY_HOST, sf1rTopology_.curNode_.host_);
    znode.setValue(ZNode::KEY_BA_PORT, sf1rTopology_.curNode_.baPort_);
    znode.setValue(ZNode::KEY_MASTER_PORT, SuperNodeManager::get()->getMasterPort());

    setServicesData(znode);

    if (zookeeper_->createZNode(serverPath_, znode.serialize(), ZooKeeper::ZNODE_EPHEMERAL_SEQUENCE))
    {
        serverRealPath_ = zookeeper_->getLastCreatedNodePath();
    }
    if (!zookeeper_->isZNodeExists(write_req_queue_parent_, ZooKeeper::WATCH))
    {
        zookeeper_->createZNode(write_req_queue_parent_);
    }
    std::vector<string> reqchild;
    zookeeper_->getZNodeChildren(write_req_queue_parent_, reqchild, ZooKeeper::WATCH);
}

void MasterManagerBase::resetAggregatorConfig()
{
    std::vector<boost::shared_ptr<AggregatorBase> >::iterator agg_it;
    for (agg_it = aggregatorList_.begin(); agg_it != aggregatorList_.end(); agg_it++)
    {
        boost::shared_ptr<AggregatorBase>& aggregator = *agg_it;

        // get shardids for collection of aggregator
        std::vector<shardid_t> shardidList;
        if (!sf1rTopology_.curNode_.master_.getShardidList(aggregator->service(),
                aggregator->collection(), shardidList))
        {
            continue;
        }

        // set workers for aggregator
        AggregatorConfig aggregatorConfig;
        for (size_t i = 0; i < shardidList.size(); i++)
        {
            WorkerMapT::iterator it = workerMap_.find(shardidList[i]);
            if (it != workerMap_.end())
            {
                if(!it->second->worker_.isGood_)
                {
                    LOG(INFO) << "worker_ : " << it->second->nodeId_ << " is not good, so do not added to aggregator.";
                    continue;
                }
                bool isLocal = (it->second->nodeId_ == sf1rTopology_.curNode_.nodeId_);
                aggregatorConfig.addWorker(it->second->host_, it->second->worker_.port_, shardidList[i], isLocal);
            }
            else
            {
                LOG (ERROR) << "worker " << shardidList[i] << " was not found for Aggregator of "
                            << aggregator->collection() << " in service " << aggregator->service();
            }
        }

        //std::cout << aggregator->collection() << ":" << std::endl << aggregatorConfig.toString();
        aggregator->setAggregatorConfig(aggregatorConfig);
    }
}

bool MasterManagerBase::isPrimaryWorker(replicaid_t replicaId, nodeid_t nodeId)
{
    std::string nodepath = getNodePath(replicaId,  nodeId);
    std::string sdata;
    if (zookeeper_->getZNodeData(nodepath, sdata, ZooKeeper::WATCH))
    {
        ZNode znode;
        znode.loadKvString(sdata);
        std::string self_reg_primary = znode.getStrValue(ZNode::KEY_SELF_REG_PRIMARY_PATH);
        std::vector<std::string> node_list;
        zookeeper_->getZNodeChildren(getPrimaryNodeParentPath(nodeId), node_list);
        if (node_list.empty())
        {
            LOG(INFO) << "no any primary node for node id: " << nodeId;
            return false;
        }
        return self_reg_primary == node_list[0];
    }
    else
        return false;
}

bool MasterManagerBase::isMinePrimary()
{
    if(!isDistributeEnable_)
        return true;

    if (!zookeeper_ || !zookeeper_->isConnected())
        return false;
    return isPrimaryWorker(sf1rTopology_.curNode_.replicaId_,  sf1rTopology_.curNode_.nodeId_);
}



