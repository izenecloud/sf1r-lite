#include "SynchroConsumer.h"

#include <node-manager/SuperNodeManager.h>

#include <util/string/StringUtils.h>

#include <sstream>

using namespace sf1r;


SynchroConsumer::SynchroConsumer(
        boost::shared_ptr<ZooKeeper>& zookeeper,
        const std::string& syncZkNode)
: zookeeper_(zookeeper)
, syncZkNode_(syncZkNode)
, consumerStatus_(CONSUMER_STATUS_INIT)
{
    if (zookeeper_)
        zookeeper_->registerEventHandler(this);

    producerZkNode_ = syncZkNode_ + SynchroZkNode::PRODUCER;
}

SynchroConsumer::~SynchroConsumer()
{
}

/**
 * Synchronizing steps for Consumer:
 *     Producer ------> ZooKeeper ---watch--> Consumer
 *     Producer <------ ZooKeeper <--notify-- Consumer : register consumer
 *
 *     Producer ------> ZooKeeper ---watch--> Consumer
 *     Producer -------receiving data-------> Consumer
 *     Producer <------ ZooKeeper <--notify-- Consumer : notify consuming status
 */
void SynchroConsumer::watchProducer(
        callback_on_produced_t callback_on_produced,
        const std::string& collectionName)
{
    consumerStatus_ = CONSUMER_STATUS_WATCHING;

    callback_on_produced_ = callback_on_produced;
    collectionName_ = collectionName;

    if (zookeeper_->isConnected())
    {
        doWatchProducer();
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

    if (zkEvent.path_ == producerZkNode_)
    {
        resetWatch();
    }
}

void SynchroConsumer::onNodeCreated(const std::string& path)
{
    if (path == producerZkNode_)
    {
        // Watched producer
        doWatchProducer();
    }
}

/*virtual*/
void SynchroConsumer::onDataChanged(const std::string& path)
{
    if (path == producerZkNode_)
    {
        // Producer status changed
        doWatchProducer();
    }
}

void SynchroConsumer::onMonitor()
{
    // Ensure connection status with ZooKeeper onTimer
    if (zookeeper_ && !zookeeper_->isConnected())
    {
        zookeeper_->connect(true);

        if (zookeeper_->isConnected())
        {
            resetWatch();
        }
    }
    else if (consumerStatus_ == CONSUMER_STATUS_WATCHING)
    {
        resetWatch();
    }
}

/// private

void SynchroConsumer::doWatchProducer()
{
    //std::cout<<"[SynchroConsumer] watch "<<producerZkNode_<<std::endl;
    boost::unique_lock<boost::mutex> lock(mutex_);

    if (consumerStatus_ == CONSUMER_STATUS_CONSUMING) {
        return;
    }
    else {
        consumerStatus_ = CONSUMER_STATUS_CONSUMING;
    }

    // Watch producer
    SynchroData producerMsg;
    if (zookeeper_->isZNodeExists(producerZkNode_, ZooKeeper::WATCH))
    {
        std::cout<<"[SynchroConsumer] watched producer: "<<producerZkNode_<<std::endl;
        std::string dataStr;
        zookeeper_->getZNodeData(producerZkNode_, dataStr, ZooKeeper::WATCH);
        producerMsg.loadKvString(dataStr);
    }
    else
    {
        consumerStatus_ = CONSUMER_STATUS_WATCHING; // reset status!
        return;
    }

    // Register consumer
    SynchroData consumerMsg;
    consumerMsg.setValue(SynchroData::KEY_CONSUMER_STATUS, SynchroData::CONSUMER_STATUS_READY);
    consumerMsg.setValue(SynchroData::KEY_COLLECTION, collectionName_);
    consumerMsg.setValue(SynchroData::KEY_HOST, SuperNodeManager::get()->getLocalHostIP());
    consumerMsg.setValue(SynchroData::KEY_DATA_PORT, SuperNodeManager::get()->getDataReceiverPort());
    if (zookeeper_->createZNode(syncZkNode_ + SynchroZkNode::CONSUMER, consumerMsg.serialize(),
            ZooKeeper::ZNODE_EPHEMERAL_SEQUENCE))
    {
        consumerNodePath_ = zookeeper_->getLastCreatedNodePath();
        //std::cout<<"[SynchroConsumer] register consumer:"<<consumerNodePath_<<std::endl;
    }

    // Synchronizing
    std::cout<<"[SynchroConsumer] receiving data "<<std::endl;
    bool ret = synchronize();

    // Post processing
    if (ret)
    {
        std::cout<<"[SynchroConsumer] processing data"<<std::endl;
        ret = consume(producerMsg);
    }

    // Notify producer with consuming status
    SynchroData consumerRes;
    std::string sret = ret ? "success" : "failure";
    consumerRes.setValue(SynchroData::KEY_RETURN, sret);
    zookeeper_->setZNodeData(consumerNodePath_, consumerRes.serialize());

    consumerStatus_ = CONSUMER_STATUS_WATCHING;
}

bool SynchroConsumer::synchronize()
{
    long step = 1;
    while (true)
    {
        // timeout?
        boost::this_thread::sleep(boost::posix_time::seconds(step));

        std::string data;
        if (zookeeper_->getZNodeData(consumerNodePath_, data))
        {
            SynchroData status;
            status.loadKvString(data);
            if (status.getStrValue(SynchroData::KEY_CONSUMER_STATUS) ==
                    SynchroData::CONSUMER_STATUS_RECEIVE_SUCCESS)
            {
                return true;
            }
            else if (status.getStrValue(SynchroData::KEY_CONSUMER_STATUS) ==
                    SynchroData::CONSUMER_STATUS_RECEIVE_FAILURE)
            {
                return false;
            }
        }
    }

    return false;
}

bool SynchroConsumer::consume(SynchroData& producerMsg)
{
    bool ret = true;

    if (callback_on_produced_)
    {
        ret = callback_on_produced_(producerMsg.getStrValue(SynchroData::KEY_DATA_PATH));
    }

    return ret;
}

void SynchroConsumer::resetWatch()
{
    zookeeper_->isZNodeExists(producerZkNode_, ZooKeeper::WATCH);

    std::string dataPath;
    zookeeper_->getZNodeData(producerZkNode_, dataPath, ZooKeeper::WATCH);
}


