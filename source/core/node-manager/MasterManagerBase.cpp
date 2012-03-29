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

void MasterManagerBase::process(ZooKeeperEvent& zkEvent)
{
    //std::cout <<CLASSNAME<<state2string(masterState_)<<" "<<zkEvent.toString();
    // xxx, handle all events here?

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
    if (masterState_ == MASTER_STATE_STARTED)
    {
        // try failover
        failover(path);
    }
}

void MasterManagerBase::onChildrenChanged(const std::string& path)
{
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

    return false;
}

void MasterManagerBase::doStart()
{
    detectReplicaSet();

    detectWorkers();

    // Register master as search server without waiting for all workers to be ready.
    // Because even if one worker is broken down and not recovered, other workers should be in serving.
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
                               << " is configured for both node " << nodeid << " and node" << workerMap_[shardid]->nodeId_ << endl;

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

                    // set worker info
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
                    LOG (ERROR) << "got wrong shardid \""<< shardid <<"\" from node [" <<  nodeid << "] @ "
                                << znode.getStrValue(ZNode::KEY_HOST)<<std::endl;
                    continue;
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
                   << "), expected " << sf1rTopology_.curNode_.master_.totalShardNum_ << std::endl;
    }

    // update config
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
        std::cout <<"Error workerPort "<<znode.getStrValue(ZNode::KEY_WORKER_PORT)
                  <<" from "<<znode.getStrValue(ZNode::KEY_HOST)<<std::endl;
    }

    try
    {
        workerNode->dataPort_ =
                boost::lexical_cast<shardid_t>(znode.getStrValue(ZNode::KEY_DATA_PORT));
    }
    catch (std::exception& e)
    {
        workerNode->worker_.isGood_ = false;
        std::cout <<"Error dataPort "<<znode.getStrValue(ZNode::KEY_DATA_PORT)
                  <<" from "<<znode.getStrValue(ZNode::KEY_HOST)<<std::endl;
    }

    LOG (INFO) << CLASSNAME << " detected worker " << workerNode->worker_.shardId_ << " "
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
        try {
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

    // new replica or new node in replica joined
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
    masterState_ = MASTER_STATE_FAILOVERING; //xxx, use lock

    // check path
    WorkerMapT::iterator it;
    for (it = workerMap_.begin(); it != workerMap_.end(); it++)
    {
        boost::shared_ptr<Sf1rNode>& sf1rNode = it->second;
        std::string nodePath = getNodePath(sf1rNode->replicaId_, sf1rNode->nodeId_);
        if (zpath == nodePath)
        {
            std::cout <<"failover, node broken "<<nodePath<<std::endl;
            if (failover(sf1rNode))
            {
                std::cout <<"failover, finished "<<std::endl;
            }
            else
            {
                std::cout <<"failover, failed to cover this failure... "<<std::endl;
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
            // try switch to replicaIdList_[i]
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
                    std::cout<<"switching node "<<sf1rNode->nodeId_<<" from replica "<<sf1rNode->replicaId_
                             <<" to "<<replicaIdList_[i]<<std::endl;
                    // xxx,
                    //sf1rNode->shardId_ = shardid;
                    //sf1rNode->nodeId_ = nodeid;
                    sf1rNode->replicaId_ = replicaIdList_[i]; // new replica
                    sf1rNode->host_ = znode.getStrValue(ZNode::KEY_HOST); //check validity xxx
                    sf1rNode->worker_.port_ = znode.getUInt32Value(ZNode::KEY_WORKER_PORT);
                    try {
                        sf1rNode->worker_.port_ =
                                boost::lexical_cast<shardid_t>(znode.getStrValue(ZNode::KEY_WORKER_PORT));
                    }
                    catch (std::exception& e)
                    {
                        std::cout <<"Error workerPort "<<znode.getStrValue(ZNode::KEY_WORKER_PORT)
                                  <<" from "<<znode.getStrValue(ZNode::KEY_HOST)<<std::endl;
                        continue;
                    }

                    sf1rNode->worker_.isGood_ = true;
                    break;
                }
                else
                {
                    //xxx
                    std::cerr<<"Error, replica inconsistent, Replica "<<replicaIdList_[i]
                             <<" Node "<<sf1rNode->nodeId_<<" has shard id "<<sf1rNode->worker_.shardId_
                             <<", but expected shard id is "<<sf1rNode->worker_.shardId_<<std::endl;
                }
            }
        }
    }

    // notify aggregators
    if (sf1rNode->worker_.isGood_)
    {
        resetAggregatorConfig();
    }

    // Watch, waiting for node recover from current replica,
    // xxx, watch all replicas, if failed and current not recovered, but another replica may recover
    zookeeper_->isZNodeExists(
            getNodePath(sf1rTopology_.curNode_.replicaId_, sf1rNode->nodeId_),
            ZooKeeper::WATCH);

    return sf1rNode->worker_.isGood_;
}


void MasterManagerBase::recover(const std::string& zpath)
{
    masterState_ = MASTER_STATE_RECOVERING; // lock?

    WorkerMapT::iterator it;
    for (it = workerMap_.begin(); it != workerMap_.end(); it++)
    {
        boost::shared_ptr<Sf1rNode>& sf1rNode = it->second;
        if (zpath == getNodePath(sf1rTopology_.curNode_.replicaId_, sf1rNode->nodeId_))
        {
            std::cout<<"Rcovering, node "<<sf1rNode->nodeId_<<" recovered in current replica "
                     <<sf1rTopology_.curNode_.replicaId_<<std::endl;

            ZNode znode;
            std::string sdata;
            if (zookeeper_->getZNodeData(zpath, sdata, ZooKeeper::WATCH))
            {
                // xxx recover

                znode.loadKvString(sdata);
                //sf1rNode->shardId_ = shardid;
                //sf1rNode->nodeId_ = nodeid;
                sf1rNode->replicaId_ = sf1rTopology_.curNode_.replicaId_; // new replica
                sf1rNode->host_ = znode.getStrValue(ZNode::KEY_HOST); //check validity xxx
                try {
                    sf1rNode->worker_.port_ =
                            boost::lexical_cast<shardid_t>(znode.getStrValue(ZNode::KEY_WORKER_PORT));
                }
                catch (std::exception& e)
                {
                    std::cout <<"Error workerPort "<<znode.getStrValue(ZNode::KEY_WORKER_PORT)
                              <<" from "<<znode.getStrValue(ZNode::KEY_HOST)<<std::endl;
                    break;
                }

                // notify aggregators
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
    // Current SF1 serve as Master

    if (!zookeeper_->isZNodeExists(serverParentPath_))
    {
        zookeeper_->createZNode(serverParentPath_);
    }

    //std::stringstream serverAddress;
    //serverAddress<<sf1rTopology_.curNode_.host_<<":"<<sf1rTopology_.curNode_.baPort_;
    ZNode znode;
    znode.setValue(ZNode::KEY_HOST, sf1rTopology_.curNode_.host_);
    znode.setValue(ZNode::KEY_BA_PORT, sf1rTopology_.curNode_.baPort_);
    znode.setValue(ZNode::KEY_MASTER_PORT, SuperNodeManager::get()->getMasterPort());
    if (zookeeper_->createZNode(serverPath_, znode.serialize(), ZooKeeper::ZNODE_EPHEMERAL_SEQUENCE))
    {
        serverRealPath_ = zookeeper_->getLastCreatedNodePath();
        // std::cout << CLASSNAME<<" Master ready to serve -- "<<serverRealPath_<<std::endl;//
    }
}

void MasterManagerBase::deregisterSearchServer()
{
    zookeeper_->deleteZNode(serverRealPath_);

    std::cout<<CLASSNAME<<" Master is boken down... " <<std::endl;
}

void MasterManagerBase::resetAggregatorConfig()
{
    //std::cout<<CLASSNAME<<" set config for "<<aggregatorList_.size()<<" aggregators"<<std::endl;

    aggregatorConfig_.reset();

    WorkerMapT::iterator it;
    for (it = workerMap_.begin(); it != workerMap_.end(); it++)
    {
        boost::shared_ptr<Sf1rNode>& sf1rNode = it->second;
        if (sf1rNode->worker_.isGood_)
        {
            bool isLocal = (sf1rNode->nodeId_ == sf1rTopology_.curNode_.nodeId_);
            aggregatorConfig_.addWorker(sf1rNode->host_, sf1rNode->worker_.port_, sf1rNode->worker_.shardId_, isLocal);
        }
    }

    std::cout << aggregatorConfig_.toString();

    // set aggregator configuration
    std::vector<net::aggregator::AggregatorBase*>::iterator agg_it;
    for (agg_it = aggregatorList_.begin(); agg_it != aggregatorList_.end(); agg_it++)
    {
        (*agg_it)->setAggregatorConfig(aggregatorConfig_);
    }
}

