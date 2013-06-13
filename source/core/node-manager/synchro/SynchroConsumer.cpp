#include "SynchroConsumer.h"

#include <node-manager/ZooKeeperNamespace.h>
#include <node-manager/SuperNodeManager.h>
#include <node-manager/DistributeFileSys.h>
#include <util/string/StringUtils.h>

#include <glog/logging.h>
#include <sstream>
#include <unistd.h> // sleep

using namespace sf1r;

#define SYNCHRO_CONSUMER "[" << syncZkNode_ << "]"

SynchroConsumer::SynchroConsumer(
        boost::shared_ptr<ZooKeeper>& zookeeper,
        const std::string& syncID)
: zookeeper_(zookeeper)
, syncID_(syncID)
, syncZkNode_(ZooKeeperNamespace::getSynchroPath() + "/" + syncID)
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
    DLOG(INFO) << SYNCHRO_CONSUMER << " process event: " << zkEvent.toString();

    if (zkEvent.type_ == ZOO_SESSION_EVENT && zkEvent.state_ == ZOO_CONNECTED_STATE)
    {
        if (consumerStatus_ == CONSUMER_STATUS_WATCHING)
            doWatchProducer();
    }

    if (zkEvent.type_ == ZOO_SESSION_EVENT && zkEvent.state_ == ZOO_EXPIRED_SESSION_STATE)
    {
        LOG(WARNING) << "SynchroConsumer node disconnected by zookeeper, state : " << zookeeper_->getStateString();
        zookeeper_->disconnect();
        zookeeper_->connect(true);
        if (zookeeper_->isConnected())
        {
            LOG(WARNING) << "SynchroConsumer node reset watch";
            resetWatch();
        }
    }

    if (zkEvent.path_ == producerZkNode_)
    {
        resetWatch();
    }
}

void SynchroConsumer::onNodeDeleted(const std::string& path)
{
    if (path == producerZkNode_)
    {
        boost::unique_lock<boost::mutex> lock(mutex_);
        LOG(INFO) << "producer deleted, stopping consuming." << path;
        consumerStatus_ = CONSUMER_STATUS_WATCHING;
    }
}

void SynchroConsumer::onNodeCreated(const std::string& path)
{
    if (path == producerZkNode_)
    {
        DLOG(INFO) << SYNCHRO_CONSUMER << " on node created: " << path;
        // Watched producer
        doWatchProducer();
    }
}

/*virtual*/
void SynchroConsumer::onDataChanged(const std::string& path)
{
    if (path == producerZkNode_)
    {
        DLOG(INFO) << SYNCHRO_CONSUMER << " on data changed: " << path;
        // Producer status changed
        doWatchProducer();
    }
}

void SynchroConsumer::onMonitor()
{
    DLOG(INFO) << SYNCHRO_CONSUMER << " on monitor";
    
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
    SynchroData producerMsg;
    {
        boost::unique_lock<boost::mutex> lock(mutex_);

        LOG(INFO) << SYNCHRO_CONSUMER << " watching for producer ...";

        if (consumerStatus_ == CONSUMER_STATUS_CONSUMING) {
            return;
        }
        else {
            consumerStatus_ = CONSUMER_STATUS_CONSUMING;
        }

        // Watch producer
        if (zookeeper_->isZNodeExists(producerZkNode_, ZooKeeper::WATCH))
        {
            LOG(INFO) << SYNCHRO_CONSUMER << " watch " << producerZkNode_;
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
            LOG(INFO) << SYNCHRO_CONSUMER << " register consumer: " << consumerNodePath_;
        }

        // Synchronizing
        LOG(INFO) << SYNCHRO_CONSUMER << " receiving data";
    }
    bool ret = synchronize();

    boost::unique_lock<boost::mutex> lock(mutex_);
    // Post processing
    if (ret)
    {
        LOG(INFO) << SYNCHRO_CONSUMER << " processing data";
        ret = consume(producerMsg);
    }

    // Notify producer with consuming status
    SynchroData consumerRes;
    std::string sret = ret ? "success" : "failure";
    consumerRes.setValue(SynchroData::KEY_RETURN, sret);
    zookeeper_->setZNodeData(consumerNodePath_, consumerRes.serialize());
    LOG(INFO) << SYNCHRO_CONSUMER << " notified producer";

    consumerStatus_ = CONSUMER_STATUS_WATCHING;
}

bool SynchroConsumer::synchronize()
{
    long step = 1;
    while (true)
    {
        // timeout?
        LOG(INFO) << SYNCHRO_CONSUMER << " sleeping for " << step << " seconds ...";
        ::sleep(step);

        boost::unique_lock<boost::mutex> lock(mutex_);
        if (consumerStatus_ == CONSUMER_STATUS_WATCHING)
        {
            LOG(WARNING) << "consuming interrupt for status changed.";
            return false;
        }
        std::string data;
        if (zookeeper_->getZNodeData(consumerNodePath_, data))
        {
            SynchroData status;
            status.loadKvString(data);
            if (status.getStrValue(SynchroData::KEY_CONSUMER_STATUS) ==
                    SynchroData::CONSUMER_STATUS_RECEIVE_SUCCESS)
            {
                LOG(INFO) << SYNCHRO_CONSUMER << "received correctly";
                return true;
            }
            else if (status.getStrValue(SynchroData::KEY_CONSUMER_STATUS) ==
                    SynchroData::CONSUMER_STATUS_RECEIVE_FAILURE)
            {
                LOG(INFO) << SYNCHRO_CONSUMER << "error on receive";
                return false;
            }
        }
        else
        {
            LOG(ERROR) << "get consumer data failed: " << consumerNodePath_;
            LOG(ERROR) << "stop synchronize.";
            return false;
        }
    }

    return false;
}

bool SynchroConsumer::consume(SynchroData& producerMsg)
{
    bool ret = true;

    if (callback_on_produced_)
    {
        DLOG(INFO) << SYNCHRO_CONSUMER << " calling consume callback";
        std::string src_path = producerMsg.getStrValue(SynchroData::KEY_DATA_PATH);
        if (producerMsg.getStrValue(SynchroData::KEY_HOST) != SuperNodeManager::get()->getLocalHostIP())
        {
            LOG(INFO) << "source path is only needed for local consume and producer.";
            LOG(INFO) << "local consume is : " << SuperNodeManager::get()->getLocalHostIP();
            LOG(INFO) << "producer is : " << producerMsg.getStrValue(SynchroData::KEY_HOST);
            if (!DistributeFileSys::get()->isEnabled())
            {
                src_path.clear();
            }
        }
        ret = callback_on_produced_(src_path);
    }

    return ret;
}

void SynchroConsumer::resetWatch()
{
    zookeeper_->isZNodeExists(producerZkNode_, ZooKeeper::WATCH);

    std::string dataPath;
    zookeeper_->getZNodeData(producerZkNode_, dataPath, ZooKeeper::WATCH);
}
