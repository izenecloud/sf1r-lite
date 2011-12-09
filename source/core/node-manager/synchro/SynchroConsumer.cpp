#include "SynchroConsumer.h"
#include "SynchroData.h"

#include <node-manager/ZkMonitor.h>

#include <util/string/StringUtils.h>

#include <sstream>

using namespace sf1r;


SynchroConsumer::SynchroConsumer(
        const std::string& zkHosts,
        int zkTimeout,
        const std::string zkSyncNodePath,
        replicaid_t replicaId,
        nodeid_t nodeId)
: replicaId_(replicaId)
, nodeId_(nodeId)
, syncNodePath_(zkSyncNodePath)
, consumerStatus_(CONSUMER_STATUS_INIT)
, replyProducer_(true)
{
    zookeeper_ = ZooKeeperManager::get()->createClient(zkHosts, zkTimeout, this, true);

    ZkMonitor::get()->addMonitorHandler(
            boost::bind(&SynchroConsumer::onMonitor, this) );

    // "/SF1R-xxxx/Synchro/ProductManager/ProducerRXNX/",
    // xxx which node to watch in distributed se?
    std::stringstream ss;
    ss<<syncNodePath_<<"/ProducerR"<<replicaId<<"N"<<nodeId;
    syncNodePath_ = ss.str();
}

void SynchroConsumer::watchProducer(
        const std::string& producerID,
        callback_on_produced_t callback_on_produced,
        bool replyProducer)
{
    consumerStatus_ = CONSUMER_STATUS_WATCHING;

    std::string producer = producerID;
    izenelib::util::Trim(producer);
    if (!producer.empty())
        syncNodePath_ += "/" + producer;

    replyProducer_ = replyProducer;
    callback_on_produced_ = callback_on_produced;

    if (zookeeper_->isConnected())
    {
        doWatchProducer();
    }
}

void SynchroConsumer::onMonitor()
{
    if (zookeeper_ && !zookeeper_->isConnected())
    {
        zookeeper_->connect(true);

        if (zookeeper_->isConnected())
            resetWatch();
    }
    else if (consumerStatus_ == CONSUMER_STATUS_WATCHING)
    {
        resetWatch();
    }
}

/*virtual*/
void SynchroConsumer::process(ZooKeeperEvent& zkEvent)
{
    //std::cout<<"[SynchroConsumer] "<< zkEvent.toString();

    if (zkEvent.type_ == ZOO_SESSION_EVENT && zkEvent.state_ == ZOO_CONNECTED_STATE)
    {
        if (consumerStatus_ == CONSUMER_STATUS_WATCHING)
            doWatchProducer();
    }

    if (zkEvent.path_ == syncNodePath_)
    {
        resetWatch();

        if (zkEvent.type_ == ZOO_CREATED_EVENT
                && consumerStatus_ == CONSUMER_STATUS_WATCHING)
            doWatchProducer();
    }
}

/*virtual*/
void SynchroConsumer::onDataChanged(const std::string& path)
{
    if (path == producerRealNodePath_)
    {
        // new data may produced
        doWatchProducer();
    }
}

/*virtual*/
void SynchroConsumer::onChildrenChanged(const std::string& path)
{
//    if (path == syncNodePath_)
//    {
//        if (consumerStatus_ == CONSUMER_STATUS_WATCHING)
//            doWatchProducer();
//    }
}

/// private

void SynchroConsumer::doWatchProducer()
{
    //std::cout<<"[SynchroConsumer] watch "<<syncNodePath_<<std::endl;
    boost::unique_lock<boost::mutex> lock(mutex_);

    // todo watch M node
    if (!zookeeper_->isZNodeExists(syncNodePath_, ZooKeeper::WATCH))
        return;

//    std::vector<std::string> childrenList;
//    zookeeper_->getZNodeChildren(syncNodePath_, childrenList, ZooKeeper::WATCH);
//
//    std::cout<<"[SynchroConsumer] watching producers: "<<syncNodePath_<<", "<<childrenList.size()<<std::endl;
//
//    if (childrenList.size() > 0)
//    {
        if (consumerStatus_ == CONSUMER_STATUS_CONSUMING)
        {
            return;
        }

        consumerStatus_ = CONSUMER_STATUS_CONSUMING;

        // Get producer info
        producerRealNodePath_ = syncNodePath_; //childrenList[0]; // xxx, 1 producer
        std::cout<<"[SynchroConsumer] watched producer: "<<producerRealNodePath_<<std::endl;
        std::string dataStr;
        SynchroData syncData;
        zookeeper_->getZNodeData(producerRealNodePath_, dataStr, ZooKeeper::WATCH);
        syncData.loadKvString(dataStr);

        // Register consumer
        std::stringstream ss;
        ss<<producerRealNodePath_<<"/ConsumerR"<<replicaId_<<"N"<<nodeId_;
        consumerRealNodePath_ = ss.str();
        //std::cout<<"register consumer:"<<consumerRealNodePath_<<std::endl;
        if (!zookeeper_->createZNode(consumerRealNodePath_, "running", ZooKeeper::ZNODE_EPHEMERAL))
        {
            // is consuming, or last consuming not finished
            return;
        }

        // Consuming
        std::cout<<"[SynchroConsumer] consuming ..."<<std::endl;

        bool ret = true;
        if (callback_on_produced_)
        {
            ret = callback_on_produced_(syncData.getStrValue(SynchroData::KEY_DATA_PATH));
        }
        if (replyProducer_)
        {
            SynchroData syncData;
            std::string sret = ret ? "success" : "failure";
            syncData.setValue(SynchroData::KEY_RETURN, sret);
            zookeeper_->setZNodeData(consumerRealNodePath_, syncData.serialize());
        }

        consumerStatus_ = CONSUMER_STATUS_WATCHING;
//    }
}

void SynchroConsumer::resetWatch()
{
    zookeeper_->isZNodeExists(syncNodePath_, ZooKeeper::WATCH);

    //std::vector<std::string> childrenList;
    //zookeeper_->getZNodeChildren(syncNodePath_, childrenList, ZooKeeper::WATCH);

    //std::string dataPath;
    //zookeeper_->getZNodeData(syncNodePath_, dataPath, ZooKeeper::WATCH);
}


