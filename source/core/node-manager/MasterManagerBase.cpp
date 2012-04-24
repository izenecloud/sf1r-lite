#include "MasterManagerBase.h"
#include "SuperNodeManager.h"

#include <boost/lexical_cast.hpp>

using namespace sf1r;

MasterManagerBase::MasterManagerBase()
: masterState_(MASTER_STATE_INIT)
, CLASSNAME("MasterManagerBase")
{
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
}

void MasterManagerBase::stop()
{
    boost::lock_guard<boost::mutex> lock(state_mutex_);
    zookeeper_->disconnect();
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
    if (zookeeper_ && zookeeper_->getZNodeData(nodePath, data))
    {
        ZNode znode;
        znode.loadKvString(data);
        znode.setValue(collection, indexStatus);
    }

}

void MasterManagerBase::process(ZooKeeperEvent& zkEvent)
{
    //std::cout <<CLASSNAME<<state2string(masterState_)<<" "<<zkEvent.toString();

    boost::lock_guard<boost::mutex> lock(state_mutex_);

    if (zkEvent.type_ == ZOO_SESSION_EVENT && zkEvent.state_ == ZOO_CONNECTED_STATE)
    {
        if (masterState_ == MASTER_STATE_STARTING_WAIT_ZOOKEEPER)
        {
            masterState_ = MASTER_STATE_STARTING;
            doStart();
        }
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
    else if (masterState_ == MASTER_STATE_STARTED
                 && masterState_ != MASTER_STATE_FAILOVERING)
    {
        // try recover
        failover(path);
    }
}

void MasterManagerBase::onNodeDeleted(const std::string& path)
{
    boost::lock_guard<boost::mutex> lock(state_mutex_);

    if (masterState_ == MASTER_STATE_STARTED)
    {
        // try failover
        failover(path);
    }
}

void MasterManagerBase::onChildrenChanged(const std::string& path)
{
    boost::lock_guard<boost::mutex> lock(state_mutex_);

    if (masterState_ > MASTER_STATE_STARTING_WAIT_ZOOKEEPER)
    {
        detectReplicaSet(path);
    }
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
    registerSearchServer();
}

int MasterManagerBase::detectWorkers()
{
    boost::lock_guard<boost::mutex> lock(workers_mutex_);

    // detect workers from "current" replica, may check other replica
    replicaid_t replicaId = sf1rTopology_.curNode_.replicaId_;

    size_t detected = 0, good = 0;
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
                shardid_t shardid = znode.getUInt32Value(ZNode::KEY_SHARD_ID);
                if (shardid > 0 && shardid <= sf1rTopology_.curNode_.master_.totalShardNum_)
                {
                    if (workerMap_.find(shardid) != workerMap_.end())
                    {
                        // check if shard id is reduplicated with existed node
                        if (workerMap_[shardid]->nodeId_ != nodeid)
                        {
                            std::stringstream ss;
                            ss << "[" << CLASSNAME << "] conflict detected: shardid " << shardid
                               << " is configured in both node" << nodeid << " and node" << workerMap_[shardid]->nodeId_ << endl;

                            throw std::runtime_error(ss.str());
                        }
                        else
                        {
                            workerMap_[shardid]->worker_.isGood_ = true;
                        }
                    }
                    else
                    {
                        // insert new worker
                        boost::shared_ptr<Sf1rNode> sf1rNode(new Sf1rNode);
                        sf1rNode->worker_.isGood_ = true;
                        workerMap_[shardid] = sf1rNode;
                    }

                    // update worker info
                    boost::shared_ptr<Sf1rNode>& workerNode = workerMap_[shardid];
                    workerNode->worker_.shardId_ = shardid;
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
                    ss << "shardid \""<< shardid << "\" in node[" << nodeid << "] @ " << znode.getStrValue(ZNode::KEY_HOST)
                       << " is out of range for current master (max shardid is " << sf1rTopology_.curNode_.master_.totalShardNum_ << ")"; 

                    //throw std::runtime_error(ss.str());
                    LOG (WARNING) << ss.str();
                }
            }
        }
        else
        {
            // reset watcher
            zookeeper_->isZNodeExists(nodePath, ZooKeeper::WATCH);
        }
    }

    if (detected >= sf1rTopology_.curNode_.master_.totalShardNum_)
    {
        masterState_ = MASTER_STATE_STARTED;
        LOG (INFO) << CLASSNAME << " detected " << sf1rTopology_.curNode_.master_.totalShardNum_
                   << " all workers (good " << good << ")" << std::endl;
    }
    else
    {
        masterState_ = MASTER_STATE_STARTING_WAIT_WORKERS;
        LOG (INFO) << CLASSNAME << " detected " << detected << " workers (good " << good
                   << "), all " << sf1rTopology_.curNode_.master_.totalShardNum_ << std::endl;
    }

    // update workers' info to aggregators
    if (good > 0)
    {
        resetAggregatorConfig();
    }

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
                boost::lexical_cast<shardid_t>(znode.getStrValue(ZNode::KEY_WORKER_PORT));
    }
    catch (std::exception& e)
    {
        workerNode->worker_.isGood_ = false;
        LOG (ERROR) << "failed to convert workerPort \"" << znode.getStrValue(ZNode::KEY_WORKER_PORT)
                    << "\" got from worker " << workerNode->worker_.shardId_
                    << " on node " << workerNode->nodeId_
                    << " @" << workerNode->host_;
    }

    try
    {
        workerNode->dataPort_ =
                boost::lexical_cast<shardid_t>(znode.getStrValue(ZNode::KEY_DATA_PORT));
    }
    catch (std::exception& e)
    {
        workerNode->worker_.isGood_ = false;
        LOG (ERROR) << "failed to convert dataPort \"" << znode.getStrValue(ZNode::KEY_DATA_PORT)
                    << "\" got from worker " << workerNode->worker_.shardId_
                    << " on node " << workerNode->nodeId_
                    << " @" << workerNode->host_;
    }

    LOG (INFO) << CLASSNAME << " detected worker" << workerNode->worker_.shardId_
               << " (node" << workerNode->nodeId_ <<") "
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
                znode.loadKvString(sdata);
                shardid_t shardid = znode.getUInt32Value(ZNode::KEY_SHARD_ID);
                if (shardid == sf1rNode->worker_.shardId_)
                {
                    LOG (INFO) << "switching node " << sf1rNode->nodeId_ << " from replica " << sf1rNode->replicaId_
                               <<" to " << replicaIdList_[i];

                    sf1rNode->replicaId_ = replicaIdList_[i]; // new replica
                    sf1rNode->host_ = znode.getStrValue(ZNode::KEY_HOST);
                    try
                    {
                        sf1rNode->worker_.port_ =
                                boost::lexical_cast<shardid_t>(znode.getStrValue(ZNode::KEY_WORKER_PORT));
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
                                << "] has shard id: " << shardid
                                << ", but corresponding shard id is " << sf1rNode->worker_.shardId_
                                << " in Replica " << sf1rNode->replicaId_;
                }
            }
        }
    }

    // notify aggregators
    if (sf1rNode->worker_.isGood_)
    {
        resetAggregatorConfig();
    }

    // Watch current replica, waiting for node recover
    zookeeper_->isZNodeExists(getNodePath(sf1rNode->replicaId_, sf1rNode->nodeId_), ZooKeeper::WATCH);

    return sf1rNode->worker_.isGood_;
}


void MasterManagerBase::recover(const std::string& zpath)
{
    masterState_ = MASTER_STATE_RECOVERING;

    WorkerMapT::iterator it;
    for (it = workerMap_.begin(); it != workerMap_.end(); it++)
    {
        boost::shared_ptr<Sf1rNode>& sf1rNode = it->second;
        if (zpath == getNodePath(sf1rTopology_.curNode_.replicaId_, sf1rNode->nodeId_))
        {
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
                            boost::lexical_cast<shardid_t>(znode.getStrValue(ZNode::KEY_WORKER_PORT));
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
                resetAggregatorConfig();
                break;
            }
        }
    }

    masterState_ = MASTER_STATE_STARTED;
}

void MasterManagerBase::registerSearchServer()
{
    // Master server provide search service
    if (!zookeeper_->isZNodeExists(serverParentPath_))
    {
        zookeeper_->createZNode(serverParentPath_);
    }

    ZNode znode;
    znode.setValue(ZNode::KEY_HOST, sf1rTopology_.curNode_.host_);
    znode.setValue(ZNode::KEY_BA_PORT, sf1rTopology_.curNode_.baPort_);
    znode.setValue(ZNode::KEY_MASTER_PORT, SuperNodeManager::get()->getMasterPort());
    if (zookeeper_->createZNode(serverPath_, znode.serialize(), ZooKeeper::ZNODE_EPHEMERAL_SEQUENCE))
    {
        serverRealPath_ = zookeeper_->getLastCreatedNodePath();
    }
}

void MasterManagerBase::resetAggregatorConfig()
{
    std::vector<boost::shared_ptr<AggregatorBase> >::iterator agg_it;
    for (agg_it = aggregatorList_.begin(); agg_it != aggregatorList_.end(); agg_it++)
    {
        boost::shared_ptr<AggregatorBase>& aggregator = *agg_it;

        // get shardids for collection of aggregator
        std::vector<shardid_t> shardidList;
        if (!sf1rTopology_.curNode_.master_.getShardidList(aggregator->collection(), shardidList))
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
                bool isLocal = (it->second->nodeId_ == sf1rTopology_.curNode_.nodeId_);
                aggregatorConfig.addWorker(it->second->host_, it->second->worker_.port_, shardidList[i], isLocal);
            }
            else
            {
                LOG (ERROR) << "worker " << shardidList[i] << " was not found for Aggregator of "
                            << aggregator->collection();
            }
        }

        //std::cout << aggregator->collection() << ":" << std::endl << aggregatorConfig.toString();
        aggregator->setAggregatorConfig(aggregatorConfig);
    }
}




