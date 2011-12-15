#include "SynchroConsumer.h"
#include "SynchroData.h"

#include <util/string/StringUtils.h>

#include <sstream>

using namespace sf1r;


SynchroConsumer::SynchroConsumer(
        boost::shared_ptr<ZooKeeper>& zookeeper,
        const std::string& syncZkNode)
: zookeeper_(zookeeper)
, syncZkNode_(syncZkNode)
, consumerStatus_(CONSUMER_STATUS_INIT)
, replyProducer_(true)
{
    if (zookeeper_)
        zookeeper_->registerEventHandler(this);
}

SynchroConsumer::~SynchroConsumer()
{
}

void SynchroConsumer::watchProducer(
        callback_on_produced_t callback_on_produced,
        bool replyProducer)
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
    //std::cout<<"[SynchroConsumer] "<< zkEvent.toString();
    if (zkEvent.type_ == ZOO_SESSION_EVENT && zkEvent.state_ == ZOO_CONNECTED_STATE)
    {
        if (consumerStatus_ == CONSUMER_STATUS_WATCHING)
            doWatchProducer();
    }

    if (zkEvent.path_ == syncZkNode_)
    {
        resetWatch();
    }
}

void SynchroConsumer::onNodeCreated(const std::string& path)
{
    if (path == syncZkNode_)
    {
        // new data may produced
        doWatchProducer();
    }
}

/*virtual*/
void SynchroConsumer::onDataChanged(const std::string& path)
{
    if (path == syncZkNode_)
    {
        // new data may produced
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

/// private

void SynchroConsumer::doWatchProducer()
{
    //std::cout<<"[SynchroConsumer] watch "<<syncZkNode_<<std::endl;
    boost::unique_lock<boost::mutex> lock(mutex_);

    // watch Procuder
    if (!zookeeper_->isZNodeExists(syncZkNode_, ZooKeeper::WATCH))
        return;

    if (consumerStatus_ == CONSUMER_STATUS_CONSUMING)
        return;

    consumerStatus_ = CONSUMER_STATUS_CONSUMING;

    // Get producer info
    std::cout<<"[SynchroConsumer] watched producer: "<<syncZkNode_<<std::endl;
    std::string dataStr;
    SynchroData syncData;
    zookeeper_->getZNodeData(syncZkNode_, dataStr, ZooKeeper::WATCH);
    syncData.loadKvString(dataStr);

    // Register consumer
    if (!zookeeper_->createZNode(syncZkNode_+"/Consumer", "running",
            ZooKeeper::ZNODE_EPHEMERAL_SEQUENCE))
    {
        // is consuming, or last consuming not finished
        return;
    }
    consumerNodePath_ = zookeeper_->getLastCreatedNodePath();
    //std::cout<<"[SynchroConsumer] register consumer:"<<consumerNodePath_<<std::endl;

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
        zookeeper_->setZNodeData(consumerNodePath_, syncData.serialize());
    }

    consumerStatus_ = CONSUMER_STATUS_WATCHING;
}

void SynchroConsumer::resetWatch()
{
    zookeeper_->isZNodeExists(syncZkNode_, ZooKeeper::WATCH);

    std::string dataPath;
    zookeeper_->getZNodeData(syncZkNode_, dataPath, ZooKeeper::WATCH);
}


