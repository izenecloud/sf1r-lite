#include "MasterNodeManager.h"
#include <aggregator-manager/AggregatorManager.h>

#include <boost/lexical_cast.hpp>

using namespace sf1r;


MasterNodeManager::MasterNodeManager()
: topology_(), curNodeInfo_(), isReady_(false)
{
}

void MasterNodeManager::initZooKeeper(const std::string& zkHosts, const int recvTimeout)
{
    zookeeper_.reset(new ZooKeeper(zkHosts, recvTimeout));
    zookeeper_->registerEventHandler(this);
}

void MasterNodeManager::setNodeInfo(Topology& topology, SF1NodeInfo& sf1NodeInfo)
{
    topology_ = topology;
    curNodeInfo_ = sf1NodeInfo;

    for (uint32_t i = 1; i <= topology_.nodeNum_; i++)
    {
        boost::shared_ptr<WorkerState> workerState(new WorkerState);
        workerState->isRunning_ = false;
        workerState->nodeId_ = i;
        workerState->zkPath_ = NodeUtil::getNodePath(curNodeInfo_.mirrorId_, i)+"/Worker";
        workerStateMap_[workerState->zkPath_] = workerState;

        if (i == curNodeInfo_.nodeId_) {
            aggregatorConfig_.enableLocalWorker_ = true; //xxx
        }
    }
}

void MasterNodeManager::startServer()
{
    isReady_ = checkWorkers();

    if (isReady_)
        registerServer();
}

void MasterNodeManager::process(ZooKeeperEvent& zkEvent)
{
    //std::cout << "MasterNodeManager::process "<<zkEvent.toString();//

    if (!isReady_)
    {
        // retry
        startServer();
    }
    else
    {
        // failure detection
    }
}

void MasterNodeManager::showWorkers()
{
    WorkerStateMapT::iterator it;
    for (it = workerStateMap_.begin(); it != workerStateMap_.end(); it++)
    {
        cout << it->second->toString() ;
    }
}

bool MasterNodeManager::checkWorkers()
{
    // xxx, lock
    if (workerStateMap_.size() <= 0)
    {
        std::cout << "No worker node!" <<std::endl;
        return false;
    }

    bool ret = true;

    WorkerStateMapT::iterator it;
    for (it = workerStateMap_.begin(); it != workerStateMap_.end(); it++)
    {
        boost::shared_ptr<WorkerState> workerState = it->second;
        if (zookeeper_->isZNodeExists(workerState->zkPath_, ZooKeeper::WATCH))
        {
            workerState->isRunning_ = true;

            std::string workerNodePath = NodeUtil::getNodePath(curNodeInfo_.mirrorId_, workerState->nodeId_);
            zookeeper_->getZNodeData(workerNodePath, workerState->host_, ZooKeeper::WATCH);

            std::string portString;
            zookeeper_->getZNodeData(workerState->zkPath_, portString, ZooKeeper::WATCH);
            try
            {
                workerState->port_ = boost::lexical_cast<unsigned int>(portString);

                aggregatorConfig_.addWorker(workerState->host_, workerState->port_);
                std::cout << "[MasterNodeManager] detected worker "<<workerState->host_<<":"<<workerState->port_<<std::endl;//
            }
            catch (std::exception& e)
            {
                std::cout <<"Failed to get worker port ("<<workerState->zkPath_<<") "<<e.what() << std::endl;
                workerState->isRunning_ = false;
                ret = false;
            }
        }
        else
        {
            workerState->isRunning_ = false;
            ret = false;
            std::cout << "[MasterNodeManager] waiting worker - "<<workerState->zkPath_<< std::endl;
        }
    }

    return ret;
}

void MasterNodeManager::registerServer()
{
    // set aggregator configuration
    std::vector<boost::shared_ptr<AggregatorManager> >::iterator it;
    for (it = aggregatorList_.begin(); it != aggregatorList_.end(); it++)
    {
        (*it)->setAggregatorConfig(aggregatorConfig_);
    }

    // This Master is ready to serve
    std::string path = NodeUtil::getSF1ServicePath();

    if (!zookeeper_->isZNodeExists(path))
    {
        zookeeper_->createZNode(path);
    }

    if (zookeeper_->createZNode(path+"/Server", "", ZooKeeper::ZNODE_EPHEMERAL_SEQUENCE))
    {
        serverPath_ = zookeeper_->getLastCreatedNodePath();

        std::cout << "[MasterNodeManager] Master ready to server -- "<<serverPath_<<std::endl;//
    }
}

