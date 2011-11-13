#include "MasterNodeManager.h"
#include "NodeManager.h"
#include "NodeData.h"

#include <aggregator-manager/AggregatorManager.h>

#include <boost/lexical_cast.hpp>

using namespace sf1r;


MasterNodeManager::MasterNodeManager()
: masterState_(MASTER_STATE_INIT)
{
}

void MasterNodeManager::init()
{
    // initialize zookeeper
    initZooKeeper(
            NodeManagerSingleton::get()->getDSUtilConfig().zkConfig_.zkHosts_,
            NodeManagerSingleton::get()->getDSUtilConfig().zkConfig_.zkRecvTimeout_);

    // initialize topology info
    topology_.clusterId_ = NodeManagerSingleton::get()->getDSTopologyConfig().clusterId_;
    topology_.nodeNum_ =  NodeManagerSingleton::get()->getDSTopologyConfig().nodeNum_;
    topology_.shardNum_ =  NodeManagerSingleton::get()->getDSTopologyConfig().shardNum_;

    curNodeInfo_ = NodeManagerSingleton::get()->getNodeInfo();

    // initialize workers,
    for (shardid_t shardid = 1; shardid <= topology_.shardNum_; shardid++)
    {
        boost::shared_ptr<WorkerNode> pworkerNode(new WorkerNode);
        workerMap_[shardid] = pworkerNode;
    }
}

void MasterNodeManager::start()
{
    // call once
    if (masterState_ == MASTER_STATE_INIT)
    {
        masterState_ = MASTER_STATE_STARTING;

        init();

        // Ensure connected to zookeeper
        if (!zookeeper_->isConnected())
        {
            zookeeper_->connect(true);

            if (!zookeeper_->isConnected())
            {
                masterState_ = MASTER_STATE_STARTING_WAIT_ZOOKEEPER;
                std::cout<<"[MasterNodeManager] waiting for ZooKeeper Service..."<<std::endl;
                return;
            }
        }

        doStart();
    }
}

void MasterNodeManager::process(ZooKeeperEvent& zkEvent)
{
    std::cout << "[MasterNodeManager] "<<zkEvent.toString();

    if (zkEvent.type_ == ZOO_SESSION_EVENT && zkEvent.state_ == ZOO_CONNECTED_STATE)
    {
        if (masterState_ == MASTER_STATE_STARTING_WAIT_ZOOKEEPER)
        {
            masterState_ = MASTER_STATE_STARTING;
            doStart();
        }
    }
}

void MasterNodeManager::onNodeCreated(const std::string& path)
{
    std::cout << "[MasterNodeManager::onNodeCreated] "<< path <<std::endl;

    if (masterState_ == MASTER_STATE_STARTING_WAIT_WORKERS)
    {
        // try detect workers
        masterState_ = MASTER_STATE_STARTING;
        detectWorkers();
    }
    else if (masterState_ > MASTER_STATE_STARTING_WAIT_ZOOKEEPER
             && masterState_ != MASTER_STATE_RECOVERING)
    {
        // try recover
        recover(path);
    }
}

void MasterNodeManager::onNodeDeleted(const std::string& path)
{
    std::cout << "[MasterNodeManager::onNodeDeleted] "<< path <<std::endl;

    if (masterState_ == MASTER_STATE_STARTED)
    {
        // try failover
        failover(path);
    }
}

void MasterNodeManager::onChildrenChanged(const std::string& path)
{
    std::cout << "[MasterNodeManager::onDataChanged] "<< path <<std::endl;

    if (masterState_ > MASTER_STATE_STARTING_WAIT_ZOOKEEPER
            && path == NodeDef::getSF1TopologyPath())
    {
        detectReplicaSet();
    }
}


void MasterNodeManager::showWorkers()
{
    WorkerMapT::iterator it;
    for (it = workerMap_.begin(); it != workerMap_.end(); it++)
    {
        cout << it->second->toString() ;
    }
}

/// private ////////////////////////////////////////////////////////////////////

void MasterNodeManager::initZooKeeper(const std::string& zkHosts, const int recvTimeout)
{
    zookeeper_.reset(new ZooKeeper(zkHosts, recvTimeout));
    zookeeper_->registerEventHandler(this);
}

void MasterNodeManager::doStart()
{
    detectReplicaSet();

    detectWorkers();

    registerSearchServer(); // notify APPs
}

int MasterNodeManager::detectWorkers()
{
    std::cout<<"[MasterNodeManager] detecting Workers ..."<<std::endl;

    // detect workers from "current replica" in topology
    size_t detected = 0;
    for (uint32_t nodeid = 1; nodeid <= topology_.nodeNum_; nodeid++)
    {
        NodeData ndata;
        std::string sdata;
        std::string nodePath = NodeDef::getNodePath(curNodeInfo_.replicaId_, nodeid);
        // get node data
        if (zookeeper_->getZNodeData(nodePath, sdata, ZooKeeper::WATCH))
        {
            ndata.loadZkData(sdata);
            if (ndata.hasKey(NDATA_KEY_WORKER_PORT))
            {
                // try { boost::lexical_cast<shardid_t>(); } xxx
                shardid_t shardid = ndata.getUIntValue(NDATA_KEY_SHARD_ID);
                if (shardid > 0 && shardid <= topology_.shardNum_)
                {
                    if (workerMap_[shardid]->nodeId_ != 0 && workerMap_[shardid]->nodeId_ != nodeid)
                    {
                        std::cout <<"Error reduplicated shardid "<<shardid
                                  <<" found for Node "<<nodeid<<" and Node "<<workerMap_[shardid]->nodeId_<<std::endl;
                        continue;
                    }

                    workerMap_[shardid]->shardId_ = shardid; // worker id
                    workerMap_[shardid]->nodeId_ = nodeid;
                    workerMap_[shardid]->replicaId_ = curNodeInfo_.replicaId_;
                    workerMap_[shardid]->host_ = ndata.getValue(NDATA_KEY_HOST); //check validity xxx
                    //workerMap_[shardid]->workerPort_ = ndata.getUIntValue(NDATA_KEY_WORKER_PORT);
                    try {
                        workerMap_[shardid]->workerPort_ =
                                boost::lexical_cast<shardid_t>(ndata.getValue(NDATA_KEY_WORKER_PORT));
                    }
                    catch (std::exception& e)
                    {
                        std::cout <<"Error workerPort "<<ndata.getValue(NDATA_KEY_WORKER_PORT)
                                  <<" from "<<ndata.getValue(NDATA_KEY_HOST)<<std::endl;
                        continue;
                    }



                    std::cout <<"detected "<<workerMap_[shardid]->toString()<<std::endl;
                    detected ++;
                }
                else
                {
                    std::cout <<"Error shardid "<<shardid<<" from "<<ndata.getValue(NDATA_KEY_HOST)<<std::endl;
                    continue;
                }
            }
        }
        else
        {
            // former watch will fail if nodePath did not existed.
            zookeeper_->isZNodeExists(nodePath, ZooKeeper::WATCH);
        }
    }

    if (detected >= workerMap_.size())
    {
        masterState_ = MASTER_STATE_STARTED;
        std::cout<<"[MasterNodeManager] all Workers are detected "<<workerMap_.size()<<std::endl;
    }
    else
    {
        masterState_ = MASTER_STATE_STARTING_WAIT_WORKERS;
        std::cout<<"[MasterNodeManager] waiting Workers to join... (detected "
                 <<detected<<", all "<<workerMap_.size()<<")"<<std::endl;
        // xxx retry times? go to failover?
    }

    // update config
    if (detected > 0)
    {
        resetAggregatorConfig();
    }

    return detected;
}

void MasterNodeManager::detectReplicaSet()
{
    // xxx, synchronize
    std::cout<<"[MasterNodeManager] detecting replicas ..."<<std::endl;

    std::vector<std::string> childrenList;
    zookeeper_->getZNodeChildren(NodeDef::getSF1TopologyPath(), childrenList, ZooKeeper::WATCH);

    replicaIdList_.clear();
    for (size_t i = 0; i < childrenList.size(); i++)
    {
        std::string sreplicaId;
        zookeeper_->getZNodeData(childrenList[i], sreplicaId);
        try {
            replicaIdList_.push_back(boost::lexical_cast<replicaid_t>(sreplicaId));
            std::cout<<"detected replica "<<replicaIdList_.back()<<std::endl;
        }
        catch (std::exception& e)
        {}
    }

    // xxx, notify if failover failed
}

void MasterNodeManager::failover(const std::string& zpath)
{
    masterState_ = MASTER_STATE_FAILOVERING; //xxx, use lock

    // check path
    WorkerMapT::iterator it;
    for (it = workerMap_.begin(); it != workerMap_.end(); it++)
    {
        boost::shared_ptr<WorkerNode>& pworkerNode = it->second;
        std::string nodePath = NodeDef::getNodePath(pworkerNode->replicaId_, pworkerNode->nodeId_);
        if (zpath == nodePath)
        {
            std::cout <<"failover, node broken "<<nodePath<<std::endl;
            if (failover(pworkerNode))
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

bool MasterNodeManager::failover(boost::shared_ptr<WorkerNode>& pworkerNode)
{
    // xxx disable master
    bool ret = false;
    for (size_t i = 0; i < replicaIdList_.size(); i++)
    {
        if (replicaIdList_[i] != pworkerNode->replicaId_)
        {
            // try switch to replicaIdList_[i]
            NodeData ndata;
            std::string sdata;
            std::string nodePath = NodeDef::getNodePath(replicaIdList_[i], pworkerNode->nodeId_);
            // get node data
            if (zookeeper_->getZNodeData(nodePath, sdata, ZooKeeper::WATCH))
            {
                ndata.loadZkData(sdata);
                shardid_t shardid = ndata.getUIntValue(NDATA_KEY_SHARD_ID);
                if (shardid == pworkerNode->shardId_)
                {
                    std::cout<<"switching node "<<pworkerNode->nodeId_<<" from replica "<<pworkerNode->replicaId_
                             <<" to "<<replicaIdList_[i]<<std::endl;
                    // xxx,
                    //pworkerNode->shardId_ = shardid;
                    //pworkerNode->nodeId_ = nodeid;
                    pworkerNode->replicaId_ = replicaIdList_[i]; // new replica
                    pworkerNode->host_ = ndata.getValue(NDATA_KEY_HOST); //check validity xxx
                    pworkerNode->workerPort_ = ndata.getUIntValue(NDATA_KEY_WORKER_PORT);

                    ret = true;
                }
            }
            else
            {
                continue;
            }
        }
    }

    // notify aggregators
    if (ret)
    {
        resetAggregatorConfig();
    }
    else
    {
        // xxx, partial search ?
        //deregisterServer();
    }

    // Watch, waiting for node recover from current replica
    zookeeper_->isZNodeExists(
            NodeDef::getNodePath(curNodeInfo_.replicaId_, pworkerNode->nodeId_),
            ZooKeeper::WATCH);

    return ret;
}


void MasterNodeManager::recover(const std::string& zpath)
{
    masterState_ = MASTER_STATE_RECOVERING; //xxx, use lock;

    WorkerMapT::iterator it;
    for (it = workerMap_.begin(); it != workerMap_.end(); it++)
    {
        boost::shared_ptr<WorkerNode>& pworkerNode = it->second;
        if (zpath == NodeDef::getNodePath(curNodeInfo_.replicaId_, pworkerNode->nodeId_))
        {
            std::cout<<"Rcovering, node "<<pworkerNode->nodeId_<<" recovered in current replica "
                     <<curNodeInfo_.replicaId_<<std::endl;

            NodeData ndata;
            std::string sdata;
            if (zookeeper_->getZNodeData(zpath, sdata, ZooKeeper::WATCH))
            {
                // xxx recover

                ndata.loadZkData(sdata);
                //pworkerNode->shardId_ = shardid;
                //pworkerNode->nodeId_ = nodeid;
                pworkerNode->replicaId_ = curNodeInfo_.replicaId_; // new replica
                pworkerNode->host_ = ndata.getUIntValue(NDATA_KEY_HOST); //check validity xxx
                pworkerNode->workerPort_ = ndata.getUIntValue(NDATA_KEY_WORKER_PORT);

                // notify aggregators
                resetAggregatorConfig();

                return;
            }
        }
    }

    masterState_ = MASTER_STATE_STARTED;
}

void MasterNodeManager::registerSearchServer()
{
    // set aggregator configuration
    std::vector<boost::shared_ptr<AggregatorManager> >::iterator it;
    for (it = aggregatorList_.begin(); it != aggregatorList_.end(); it++)
    {
        (*it)->setAggregatorConfig(aggregatorConfig_);
    }

    // This Master is ready to serve
    std::string path = NodeDef::getSF1ServicePath();

    if (!zookeeper_->isZNodeExists(path))
    {
        zookeeper_->createZNode(path);
    }

    std::stringstream serverAddress;
    serverAddress<<curNodeInfo_.host_<<":"<<curNodeInfo_.baPort_;
    if (zookeeper_->createZNode(path+"/Server", serverAddress.str(), ZooKeeper::ZNODE_EPHEMERAL_SEQUENCE))
    {
        serverPath_ = zookeeper_->getLastCreatedNodePath();

        std::cout << "[MasterNodeManager] Master ready to serve -- "<<serverPath_<<std::endl;//
    }
}

void MasterNodeManager::deregisterSearchServer()
{
    zookeeper_->deleteZNode(serverPath_);

    std::cout << "[MasterNodeManager] Master is boken down... " <<std::endl;
}

void MasterNodeManager::resetAggregatorConfig()
{
    std::cout << "[MasterNodeManager::resetAggregatorConfig] "<<std::endl;

    aggregatorConfig_.reset();

    WorkerMapT::iterator it;
    for (it = workerMap_.begin(); it != workerMap_.end(); it++)
    {
        boost::shared_ptr<WorkerNode>& pworkerNode = it->second;

        if (pworkerNode->nodeId_ == curNodeInfo_.nodeId_)
        {
            aggregatorConfig_.enableLocalWorker_ = true;
        }
        else
        {
            aggregatorConfig_.addWorker(pworkerNode->host_, pworkerNode->workerPort_);
        }
    }

    std::cout << aggregatorConfig_.toString()<<std::endl;

    // set aggregator configuration
    std::vector<boost::shared_ptr<AggregatorManager> >::iterator agg_it;
    for (agg_it = aggregatorList_.begin(); agg_it != aggregatorList_.end(); agg_it++)
    {
        (*agg_it)->setAggregatorConfig(aggregatorConfig_);

        ///std::cout << aggregatorConfig_.toString() <<std::endl;
    }
}

