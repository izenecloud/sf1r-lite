#include "SynchroConsumer.h"

#include <sstream>

using namespace sf1r;
using namespace zookeeper;


SynchroConsumer::SynchroConsumer(
        const std::string& zkHosts,
        int zkTimeout,
        const std::string zkSyncNodePath,
        replicaid_t replicaId,
        nodeid_t nodeId)
:replicaId_(replicaId), nodeId_(nodeId), syncNodePath_(zkSyncNodePath), consumerStatus_(CONSUMER_STATUS_INIT), replyProducer_(true)
{
    zookeeper_.reset(new ZooKeeper(zkHosts, zkTimeout, true));
    zookeeper_->registerEventHandler(this);

    // "/SF1R-xxxx/Synchro/ProductManager/ProducerRXNX/"
    //std::stringstream ss;
    //ss<<syncNodePath_<<"/ProducerR"<<replicaId<<"N"<<nodeId;
}


void SynchroConsumer::watchProducer(callback_on_produced_t callback_on_produced, bool replyProducer)
{
    consumerStatus_ = CONSUMER_STATUS_WATCHING;
    replyProducer_ = replyProducer;

    callback_on_produced_ = callback_on_produced;

    if (zookeeper_->isConnected())
    {
        doWatchProducer();
    }
}

/*virtual*/
void SynchroConsumer::process(ZooKeeperEvent& zkEvent)
{
    std::cout<<"[SynchroConsumer::process] "<< zkEvent.toString();
    if (zkEvent.type_ == ZOO_SESSION_EVENT && zkEvent.state_ == ZOO_CONNECTED_STATE)
    {
        if (consumerStatus_ == CONSUMER_STATUS_WATCHING)
            doWatchProducer();
    }

    resetWatch();
}

/*virtual*/
void SynchroConsumer::onDataChanged(const std::string& path)
{
    //std::cout<<"[SynchroConsumer::onDataChanged] "<< path<<std::endl;
    if (path == producerRealNodePath_)
    {
        // new data may produced
        doWatchProducer();
    }
}

/*virtual*/
void SynchroConsumer::onChildrenChanged(const std::string& path)
{
    //std::cout<<"[SynchroConsumer::onChildrenChanged] "<< path<<std::endl;
    if (path == syncNodePath_)
    {
        if (consumerStatus_ == CONSUMER_STATUS_WATCHING)
            doWatchProducer();
    }
}

/// private

void SynchroConsumer::doWatchProducer()
{
    //std::cout<<"[SynchroConsumer::doWatchProducer]... "<<std::endl;
    boost::unique_lock<boost::mutex> lock(mutex_);

    // todo watch M node
    std::vector<std::string> childrenList;
    zookeeper_->getZNodeChildren(syncNodePath_, childrenList, ZooKeeper::WATCH);

    std::cout<<"[SynchroConsumer::doWatchProducer] watching children of: "<<syncNodePath_<<", "<<childrenList.size()<<std::endl;

    if (childrenList.size() > 0)
    {
        consumerStatus_ = CONSUMER_STATUS_CONSUMING;

        // Get producer info
        producerRealNodePath_ = childrenList[0]; // xxx, 1 producer
        std::cout<<"watched:"<<producerRealNodePath_<<std::endl;
        std::string dataPath;
        zookeeper_->getZNodeData(producerRealNodePath_, dataPath, ZooKeeper::WATCH);

        // Register consumer
        std::stringstream ss;
        ss<<producerRealNodePath_<<"/ConsumerR"<<replicaId_<<"N"<<nodeId_;
        consumerRealNodePath_ = ss.str();
        std::cout<<"register consumer:"<<consumerRealNodePath_<<std::endl;
        if (!zookeeper_->createZNode(consumerRealNodePath_, "running", ZooKeeper::ZNODE_EPHEMERAL))
        {
            // is consuming, or
            return;
        }

        // Consuming
        bool ret = callback_on_produced_(dataPath);
        if (replyProducer_)
        {
            std::string retstr = ret ? "success" : "failure";
            zookeeper_->setZNodeData(consumerRealNodePath_, retstr);
        }

        consumerStatus_ = CONSUMER_STATUS_WATCHING;
    }
}

void SynchroConsumer::resetWatch()
{
    std::vector<std::string> childrenList;
    zookeeper_->getZNodeChildren(syncNodePath_, childrenList, ZooKeeper::WATCH);

    std::string dataPath;
    zookeeper_->getZNodeData(producerRealNodePath_, dataPath, ZooKeeper::WATCH);
}


