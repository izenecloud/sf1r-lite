#include "MasterManagerBase.h"
#include "SuperNodeManager.h"

#include <boost/lexical_cast.hpp>

using namespace sf1r;

MasterManagerBase::MasterManagerBase()
: masterState_(MASTER_STATE_INIT)
, CLASSNAME("[MasterManagerBase]")
{
}

void MasterManagerBase::start()
{
    // call once
    if (masterState_ == MASTER_STATE_INIT)
    {
        masterState_ = MASTER_STATE_STARTING;

        if (!init())
        {
            std::cerr<<CLASSNAME<<" Initialize failed!"<<std::endl;
            return;
        }

        // Ensure connected to zookeeper
        if (!zookeeper_->isConnected())
        {
            zookeeper_->connect(true);

            if (!zookeeper_->isConnected())
            {
                masterState_ = MASTER_STATE_STARTING_WAIT_ZOOKEEPER;
                std::cout<<CLASSNAME<<" waiting for ZooKeeper Service..."<<std::endl;
                return;
            }
        }

        doStart();
    }
}

void MasterManagerBase::stop()
{
    // stop events on exit
    zookeeper_->disconnect();
}

bool MasterManagerBase::getShardReceiver(
        unsigned int shardid,
        std::string& host,
        unsigned int& recvPort)
{
    boost::lock_guard<boost::mutex> lock(mutex_);

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

void MasterManagerBase::doStart()
{
    detectReplicaSet();

    detectWorkers();

    // Register SF1, who works as Master, as search server without
    // waiting for all workers to be ready. Because even if one worker is
    // broken down and not recovered, other workers should be in serving.
    registerSearchServer();
}

int MasterManagerBase::detectWorkers()
{
    boost::lock_guard<boost::mutex> lock(mutex_);
    //std::cout<<CLASSNAME<<" detecting Workers ..."<<std::endl;

    // detect workers from "current" replica
    size_t detected = 0, good = 0;
    for (uint32_t nodeid = 1; nodeid <= sf1rTopology_.nodeNum_; nodeid++)
    {
        ZNode znode;
        std::string sdata;
        std::string nodePath = getNodePath(sf1rTopology_.curNode_.replicaId_, nodeid);
        // get node data
        if (zookeeper_->getZNodeData(nodePath, sdata, ZooKeeper::WATCH))
        {
            znode.loadKvString(sdata);
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
                            workerMap_[shardid]->worker_.isGood_ = false;
                            std::cout <<"Error, shardid "<<shardid<<" for Node "<<nodeid
                                      <<" is the same with Node "<<workerMap_[shardid]->nodeId_<<std::endl;
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
                    workerMap_[shardid]->worker_.shardId_ = shardid; // worker id
                    workerMap_[shardid]->nodeId_ = nodeid;
                    workerMap_[shardid]->replicaId_ = sf1rTopology_.curNode_.replicaId_;
                    workerMap_[shardid]->host_ = znode.getStrValue(ZNode::KEY_HOST); //check validity xxx
                    //workerMap_[shardid]->workerPort_ = znode.getUInt32Value(ZNode::KEY_WORKER_PORT);
                    try {
                        workerMap_[shardid]->worker_.workerPort_ =
                                boost::lexical_cast<shardid_t>(znode.getStrValue(ZNode::KEY_WORKER_PORT));
                    }
                    catch (std::exception& e)
                    {
                        workerMap_[shardid]->worker_.isGood_ = false;
                        std::cout <<"Error workerPort "<<znode.getStrValue(ZNode::KEY_WORKER_PORT)
                                  <<" from "<<znode.getStrValue(ZNode::KEY_HOST)<<std::endl;
                    }
                    try {
                        workerMap_[shardid]->dataPort_ =
                                boost::lexical_cast<shardid_t>(znode.getStrValue(ZNode::KEY_DATA_PORT));
                    }
                    catch (std::exception& e)
                    {
                        workerMap_[shardid]->worker_.isGood_ = false;
                        std::cout <<"Error dataPort "<<znode.getStrValue(ZNode::KEY_DATA_PORT)
                                  <<" from "<<znode.getStrValue(ZNode::KEY_HOST)<<std::endl;
                    }

                    // xxx check more info?
                    std::cout <<CLASSNAME<<" detected "<<workerMap_[shardid]->toString()<<std::endl;
                    detected ++;
                    if (workerMap_[shardid]->worker_.isGood_)
                        good ++;
                }
                else
                {
                    std::cout <<"Error shardid "<<shardid<<" from "<<znode.getStrValue(ZNode::KEY_HOST)<<std::endl;
                    continue;
                }
            }
        }
        else
        {
            // todo, check replica node ?

            // former watch will fail if nodePath did not existed.
            zookeeper_->isZNodeExists(nodePath, ZooKeeper::WATCH);
        }
    }

    if (detected >= sf1rTopology_.curNode_.master_.totalShardNum_)
    {
        masterState_ = MASTER_STATE_STARTED;
        std::cout<<CLASSNAME<<" all Workers are detected "<<sf1rTopology_.curNode_.master_.totalShardNum_
                 <<" (good "<<good<<")"<<std::endl;
    }
    else
    {
        masterState_ = MASTER_STATE_STARTING_WAIT_WORKERS;
        std::cout<<CLASSNAME<<" detected "<<detected
                 <<" worker(s) (good "<<good<<"), all "<<sf1rTopology_.curNode_.master_.totalShardNum_<<" - waiting"<<std::endl;
    }

    // update config
    if (good > 0)
    {
        resetAggregatorConfig();
    }

    return good;
}

void MasterManagerBase::detectReplicaSet(const std::string& zpath)
{
    //std::cout<<CLASSNAME<<" detecting replicas ..."<<std::endl;

    // xxx synchronize
    std::vector<std::string> childrenList;
    zookeeper_->getZNodeChildren(topologyPath_, childrenList, ZooKeeper::WATCH);

    replicaIdList_.clear();
    for (size_t i = 0; i < childrenList.size(); i++)
    {
        std::string sreplicaId;
        zookeeper_->getZNodeData(childrenList[i], sreplicaId);
        try {
            replicaIdList_.push_back(boost::lexical_cast<replicaid_t>(sreplicaId));
            ///std::cout<<CLASSNAME<<" detected replica "<<replicaIdList_.back()<<std::endl;
        }
        catch (std::exception& e)
        {}

        // watch for nodes change
        std::vector<std::string> chchList;
        zookeeper_->getZNodeChildren(childrenList[i], chchList, ZooKeeper::WATCH);
        zookeeper_->isZNodeExists(childrenList[i],ZooKeeper::WATCH);
    }

    // new replica or new node in replica joined
    // xxx, notify if failover failed
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
                    sf1rNode->worker_.workerPort_ = znode.getUInt32Value(ZNode::KEY_WORKER_PORT);
                    try {
                        sf1rNode->worker_.workerPort_ =
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
                    sf1rNode->worker_.workerPort_ =
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
            aggregatorConfig_.addWorker(sf1rNode->host_, sf1rNode->worker_.workerPort_, sf1rNode->worker_.shardId_, isLocal);
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

