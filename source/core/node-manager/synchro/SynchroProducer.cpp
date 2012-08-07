#include "SynchroProducer.h"

#include <node-manager/SuperNodeManager.h>
#include <net/distribute/DataTransfer2.hpp>

#include <boost/thread.hpp>
#include <glog/logging.h>
#include <sstream>

using namespace sf1r;

#define SYNCHRO_PRODUCER "[" << syncZkNode_ << "]"

SynchroProducer::SynchroProducer(
        boost::shared_ptr<ZooKeeper>& zookeeper,
        const std::string& syncZkNode,
        DataTransferPolicy transferPolicy)
: transferPolicy_(transferPolicy)
, zookeeper_(zookeeper)
, syncZkNode_(syncZkNode)
, isSynchronizing_(false)
{
    if (zookeeper_)
        zookeeper_->registerEventHandler(this);

    producerZkNode_ = syncZkNode_ + SynchroZkNode::PRODUCER;

    init();
}

SynchroProducer::~SynchroProducer()
{
    zookeeper_->deleteZNode(syncZkNode_, true);
}

/**
 * Synchronizing steps for Producer:
 * (1) doProduce()
 *     Producer ---notify--> ZooKeeper -----> Consumer(s)
 * (2) watchConsumers():
 *     Producer <--watch--- ZooKeeper <----- Consumer
 *     Producer -----transfer data----> Consumer
 *     Producer ---notify--> ZooKeeper -----> Consumer
 * (3) checkConsumers()
 *     Producer <--watch--- ZooKeeper <----- Consumer
 */
bool SynchroProducer::produce(SynchroData& syncData, callback_on_consumed_t callback_on_consumed)
{
    boost::lock_guard<boost::mutex> lock(produce_mutex_);

    // start synchronizing
    if (isSynchronizing_)
    {
        LOG(ERROR) << SYNCHRO_PRODUCER << " is synchronizing!";
        return false;
    }
    else
    {
        isSynchronizing_ = true;
        syncData_ = syncData;
        init();
    }

    // produce
    if (doProduce(syncData))
    {
        if (callback_on_consumed != NULL)
            callback_on_consumed_ = callback_on_consumed;

        watchConsumers(); // wait consumer(s)
        checkConsumers(); // check consuming status
        return true;
    }
    else
    {
        endSynchroning("synchronize error or timeout!");
        return false;
    }
}

bool SynchroProducer::wait(int timeout)
{
    if (!isSynchronizing_)
    {
        LOG(INFO) << SYNCHRO_PRODUCER << " wait not synchronizing, call produce() first.";
        return false;
    }

    // wait consumer(s) to consume produced data
    int step = 1;
    int waited = 0;
    LOG(INFO) << SYNCHRO_PRODUCER << " waiting for consumer (timeout: " << timeout << ")";
    while (!watchedConsumer_)
    {
        LOG(INFO) << SYNCHRO_PRODUCER << " sleeping for " << step << " seconds ...";
        boost::this_thread::sleep(boost::posix_time::seconds(step));
        waited += step;
        if (waited >= timeout)
        {
            endSynchroning("timeout: no consumer!");
            return false;
        }
    }

    // wait synchronizing to finish
    while (isSynchronizing_)
    {
        LOG(INFO) << SYNCHRO_PRODUCER << " is synchronizing, "
                  << "sleeping for 1 second ...";
        boost::this_thread::sleep(boost::posix_time::seconds(1));
    }

    return result_on_consumed_;
}

/* virtual */
void SynchroProducer::process(ZooKeeperEvent& zkEvent)
{
    DLOG(INFO) << SYNCHRO_PRODUCER << "process event: "<< zkEvent.toString();
}

void SynchroProducer::onNodeDeleted(const std::string& path)
{
    if (consumersMap_.find(path) != consumersMap_.end())
    {
        DLOG(INFO) << SYNCHRO_PRODUCER << " on node deleted: " << path;
        // check if consumer broken down
        checkConsumers();
    }
}
void SynchroProducer::onDataChanged(const std::string& path)
{
    if (consumersMap_.find(path) != consumersMap_.end())
    {
        DLOG(INFO) << SYNCHRO_PRODUCER << " on data changed: " << path;
        // check if consumer finished
        checkConsumers();
    }
}
void SynchroProducer::onChildrenChanged(const std::string& path)
{
    if (path == syncZkNode_)
    {
        DLOG(INFO) << SYNCHRO_PRODUCER << " on children changed: " << path;
        // whether new consumer comes, or consumer break down.
        watchConsumers();
        checkConsumers();
    }
}

/// private

bool SynchroProducer::doProduce(SynchroData& syncData)
{
    bool produced = false;

    if (zookeeper_->isConnected())
    {
        // ensure synchro node
        zookeeper_->createZNode(syncZkNode_);

        // create producer node (ephemeral)
        if (!zookeeper_->createZNode(producerZkNode_, syncData.serialize(), ZooKeeper::ZNODE_EPHEMERAL))
        {
            if (zookeeper_->getErrorCode() == ZooKeeper::ZERR_ZNODEEXISTS)
            {
                zookeeper_->setZNodeData(producerZkNode_, syncData.serialize());
                LOG(WARNING) << SYNCHRO_PRODUCER << " overwrite " << producerZkNode_;
                produced = true;
            }

            LOG(INFO) << SYNCHRO_PRODUCER << " failed to create " << producerZkNode_
                      << " (" << zookeeper_->getErrorString() << ")";

            // wait ZooKeeperManager to init
            LOG(INFO) << SYNCHRO_PRODUCER << " waiting for znode initialization";
            sleep(10);
        }
        else
        {
            LOG(INFO) << SYNCHRO_PRODUCER << " created " << producerZkNode_;
            produced = true;
        }
    }

    if (!produced)
    {
        int retryCnt = 0;
        while (!zookeeper_->isConnected())
        {
            LOG(INFO) << SYNCHRO_PRODUCER << " connecting to ZooKeeper (" 
                      << zookeeper_->getHosts() << ")";
            zookeeper_->connect(true);
            if ((retryCnt++) > 10)
                break;
        }

        if (!zookeeper_->isConnected())
        {
            LOG(INFO) << SYNCHRO_PRODUCER << " connect to ZooKeeper timeout!";
        }
        else
        {
            produced = doProduce(syncData);
        }
    }

    return produced;
}

void SynchroProducer::watchConsumers()
{
    boost::unique_lock<boost::mutex> lock(consumers_mutex_);

    if (!isSynchronizing_)
        return;

    LOG(INFO) << SYNCHRO_PRODUCER << " watching for consumers";
    // [Synchro Node]
    //  |--- Producer
    //  |--- Consumer00000000
    //  |--- Consumer0000000x
    //
    std::vector<std::string> childrenList;
    zookeeper_->getZNodeChildren(syncZkNode_, childrenList, ZooKeeper::WATCH);

    // If watched any consumer
    if (childrenList.size() > 1)
    {
        LOG(INFO) << SYNCHRO_PRODUCER << " found (" << childrenList.size() << ") children";
        if (!watchedConsumer_)
            watchedConsumer_ = true;

        for (size_t i = 0; i < childrenList.size(); i++)
        {
            // Produer is also a child and shoud be excluded.
            if (producerZkNode_ != childrenList[i] &&
                consumersMap_.find(childrenList[i]) == consumersMap_.end())
            {
                consumersMap_[childrenList[i]] = std::make_pair(false, false);
                LOG(INFO) << SYNCHRO_PRODUCER << " watched a consumer: " << childrenList[i];
            }
        }
    }

    // transfer data
    consumermap_t::iterator it;
    for (it = consumersMap_.begin(); it != consumersMap_.end(); it++)
    {
        if (!transferData(it->first))
        {
            // xxx set failed status
            LOG(WARNING) << SYNCHRO_PRODUCER << " set failed status";
        }
    }
}

bool SynchroProducer::transferData(const std::string& consumerZnodePath)
{
    std::string data;
    if (!zookeeper_->getZNodeData(consumerZnodePath, data))
    {
        return false;
    }

    // consumer info
    SynchroData consumerInfo;
    consumerInfo.loadKvString(data);
    std::string consumerHost = consumerInfo.getStrValue(SynchroData::KEY_HOST);
    std::string consumerCollection = consumerInfo.getStrValue(SynchroData::KEY_COLLECTION);

    // produced info
    std::string dataPath = syncData_.getStrValue(SynchroData::KEY_DATA_PATH);
    std::string dataType = syncData_.getStrValue(SynchroData::KEY_DATA_TYPE);

    bool ret = true;
    // to local host
    if (consumerHost == SuperNodeManager::get()->getLocalHostIP())
    {
        LOG(INFO) << SYNCHRO_PRODUCER << " consumerHost: " << consumerHost << " is on localhost";
        ret = true;
    }
    // to remote host
    else
    {
        if (transferPolicy_ == DATA_TRANSFER_POLICY_DFS)
        {
            // xxx
            ret = true;
        }
        else if (transferPolicy_ == DATA_TRANSFER_POLICY_SOCKET)
        {
            uint32_t consumerPort = consumerInfo.getUInt32Value(SynchroData::KEY_DATA_PORT);
            std::string recvDir;
            if (dataType == SynchroData::DATA_TYPE_SCD_INDEX)
            {
                recvDir = consumerCollection+"/scd/index";
            }
            else
            {
                //xxx extend
            }

            LOG(INFO) << SYNCHRO_PRODUCER << " transfer data " << dataPath
                      << " to " << consumerHost << ":" <<consumerPort;
            izenelib::net::distribute::DataTransfer2 transfer(consumerHost, consumerPort);
            if (not transfer.syncSend(dataPath, recvDir, false))
            {
                ret = false;
            }
        }
    }

    // notify consumer after transferred
    const char* status;
    if (ret) {
        status = SynchroData::CONSUMER_STATUS_RECEIVE_SUCCESS;
    } else {
        status = SynchroData::CONSUMER_STATUS_RECEIVE_FAILURE;
    }

    LOG(INFO) << SYNCHRO_PRODUCER << " setting consumer status to " << status;
    consumerInfo.setValue(SynchroData::KEY_CONSUMER_STATUS, status);
    zookeeper_->setZNodeData(consumerZnodePath, consumerInfo.serialize());
    return ret;
}

void SynchroProducer::checkConsumers()
{
    LOG(INFO) << SYNCHRO_PRODUCER << " checking consumers";
    boost::unique_lock<boost::mutex> lock(consumers_mutex_);

    if (!isSynchronizing_)
        return;

    LOG(INFO) << SYNCHRO_PRODUCER << " consumers map size: " << consumersMap_.size();
    consumermap_t::iterator it;
    for (it = consumersMap_.begin(); it != consumersMap_.end(); it++)
    {
        if (it->second.first == true)
        {   // have got result
            continue;
        }

        const std::string& consumer = it->first;
        DLOG(INFO) << SYNCHRO_PRODUCER << " checking consumer " << consumer;
        std::string sdata;
        if (zookeeper_->getZNodeData(consumer, sdata, ZooKeeper::WATCH))
        {
            DLOG(INFO) << SYNCHRO_PRODUCER << " data: " << sdata;
            SynchroData syncData;
            syncData.loadKvString(sdata);
            std::string syncRet = syncData.getStrValue(SynchroData::KEY_RETURN);
            if (syncRet == "success")
            {
                it->second.first = true;  // got consumer result
                it->second.second = true; // result is true
            }
            else if (syncRet == "failure")
            {
                it->second.first = true;  // got consumer result
                it->second.second = false; // result is false
            }
            else
                continue;

            LOG(INFO) << SYNCHRO_PRODUCER << " " << syncRet << " on consumer " << consumer;
                    
            consumedCount_ ++; // how many consumers have finished
            zookeeper_->deleteZNode(consumer); // delete node after have got result.
            LOG(INFO) << SYNCHRO_PRODUCER << " deleted node " << consumer;
        }
        else
        {
            if (!zookeeper_->isZNodeExists(consumer))
            {
                it->second.first = true;
                it->second.second = false;
                consumedCount_ ++;

                LOG(WARNING) << SYNCHRO_PRODUCER << " lost connection to " << consumer<< "!!";
            }
        }
    }

    if (consumedCount_ <= 0)
    {
        return;
    }
    else
    {
        LOG(INFO) << SYNCHRO_PRODUCER << " consumed by "<< consumedCount_
                  << "/" << consumersMap_.size() << " consumers";
    }

    if (consumedCount_ == consumersMap_.size())
    {
        result_on_consumed_ = true;
        std::string info = "process succeeded";
        consumermap_t::iterator it;
        for (it = consumersMap_.begin(); it != consumersMap_.end(); it++)
        {
            // xxx, if one of the consumers failed
            if (it->second.second == false)
            {
                result_on_consumed_ = false;
                info = "process failed";
                break;
            }
        }

        if (callback_on_consumed_) {
            DLOG(INFO) << SYNCHRO_PRODUCER << " calling completion callback";
            callback_on_consumed_(result_on_consumed_);
        }

        // Synchronizing finished
        endSynchroning(info);
    }
}

void SynchroProducer::init()
{
    watchedConsumer_ = false;
    consumersMap_.clear();
    consumedCount_ = 0;
    callback_on_consumed_ = NULL;
    result_on_consumed_ = false;
}


void SynchroProducer::endSynchroning(const std::string& info)
{
    // Synchronizing finished
    isSynchronizing_ = false;
    zookeeper_->deleteZNode(syncZkNode_, true);
    LOG(INFO) << SYNCHRO_PRODUCER << " synchronizing finished - "<< info;
}