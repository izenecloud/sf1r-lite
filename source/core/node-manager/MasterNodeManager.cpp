#include "MasterNodeManager.h"


using namespace sf1r;


MasterNodeManager::MasterNodeManager()
: isReady_(false), topology_()
{
}

void MasterNodeManager::initZooKeeper(const std::string& zkHosts, const int recvTimeout)
{
    zookeeper_.reset(new ZooKeeper(zkHosts, recvTimeout));
    zookeeper_->registerEventHandler(this);
}

void MasterNodeManager::initTopology(uint32_t nodeNum, uint32_t mirrorNum)
{
    topology_.nodeNum_ = nodeNum;
    topology_.mirrorNum_ = mirrorNum;
}

void MasterNodeManager::process(ZooKeeperEvent& zkEvent)
{
    checkWorkers();
}

void MasterNodeManager::onMasterReady()
{
    // xxx, add workers to AggregatorManagers.
}

void MasterNodeManager::checkWorkers()
{
    // check worker status
}

void MasterNodeManager::registerMasterNode()
{

}
