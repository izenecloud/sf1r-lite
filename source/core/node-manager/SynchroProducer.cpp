#include "SynchroProducer.h"

#include <node-manager/NodeDef.h>

#include <sstream>

using namespace sf1r;
using namespace zookeeper;

SynchroProducer::SynchroProducer(
        const std::string& zkHosts,
        int zkTimeout,
        const std::string zkSyncNodePath,
        replicaid_t replicaId,
        nodeid_t nodeId)
: syncNodePath_(zkSyncNodePath), isSynchronizing_(false)
{
    init();

    zookeeper_.reset(new ZooKeeper(zkHosts, zkTimeout, true));
    zookeeper_->registerEventHandler(this);

    // "/SF1R-xxxx/Synchro/ProductManager/ProducerRXNX"
    std::stringstream ss;
    ss<<syncNodePath_<<"/ProducerR"<<replicaId<<"N"<<nodeId;
    prodNodePath_ = ss.str();
}

bool SynchroProducer::produce(const std::string& dataPath, callback_on_consumed_t callback_on_consumed)
{
    if (isSynchronizing_)
    {
        // xxx
        std::cout<<"[SynchroProducer::produce] Re-entry error: synchronizer is working !"<<std::endl;
        return false;
    }
    else
    {
        isSynchronizing_ = true;
        init();
    }

    if (doProcude(dataPath))
    {
        if (callback_on_consumed != NULL)
        {
            callback_on_consumed_ = callback_on_consumed;
        }

        // watching consumers
        watchConsumers();
        checkConsumers();

        return true;
    }
    return false;
}

bool SynchroProducer::waitConsumers(bool& isSuccess, int findConsumerTimeout)
{
    //std::cout<<"SynchroProducer::waitConsumers"<<std::endl;

    if (!isSynchronizing_)
    {
        std::cout<<"[SynchroProducer::waitConsumers] no synchronizing working.."<<std::endl;
        return false;
    }

    int step = 1;
    int waited = 0;
    while (!watchedConsumer_)
    {
        sleep(step);
        waited += step;
        if (waited >= findConsumerTimeout)
        {
            std::cout<<"Timeout, not saw any consumer."<<std::endl;
            return false;
        }
    }

    while (isSynchronizing_)
    {
        sleep(1);
    }

    isSuccess = result_on_consumed_;
    return true;
}

/* virtual */
void SynchroProducer::process(ZooKeeperEvent& zkEvent)
{
//    std::cout<<"SynchroProducer::process "<< zkEvent.toString();
//
//    if (zkEvent.type_ == ZOO_SESSION_EVENT && zkEvent.state_ == ZOO_CONNECTED_STATE)
//    {
//        //std::cout<<"Connected to zookeeper."<<std::endl;
//        // xxx, delay processes when zookeeper connected
//    }
//    else if (zkEvent.type_ == ZOO_SESSION_EVENT && zkEvent.state_ == ZOO_CONNECTING_STATE)
//    {
//        //std::cout<<"Connecting to zookeeper (not started or broken down)."<<std::endl;
//    }
}

void SynchroProducer::onNodeDeleted(const std::string& path)
{
    //std::cout<<"SynchroProducer::onNodeDeleted "<< path<<std::endl;

    if (consumersMap_.find(path) != consumersMap_.end())
    {
        // check if consumer broken down
        checkConsumers();
    }
}
void SynchroProducer::onDataChanged(const std::string& path)
{
    //std::cout<<"SynchroProducer::onDataChanged "<< path<<std::endl;

    if (consumersMap_.find(path) != consumersMap_.end())
    {
        // check if consumer finished
        checkConsumers();
    }
}
void SynchroProducer::onChildrenChanged(const std::string& path)
{
    //std::cout<<"SynchroProducer::onChildrenChanged "<< path<<std::endl;

    if (path == prodNodePath_)
    {
        // whether new consumer comes, or consumer break down.
        watchConsumers();
        checkConsumers();
    }
}

/// private

bool SynchroProducer::doProcude(const std::string& dataPath)
{
    if (zookeeper_->isConnected())
    {
        ensureParentNodes();

        if (!zookeeper_->createZNode(prodNodePath_, dataPath))
        {
            if (zookeeper_->getErrorCode() == ZooKeeper::ZERR_ZNODEEXISTS)
            {
                zookeeper_->setZNodeData(prodNodePath_, dataPath);
                std::cout<<"[SynchroProducer::produce] overwrote "<<prodNodePath_<<" : "<<dataPath<<std::endl;
                return true;
            }

            std::cout<<"[SynchroProducer::produce] failed to create "<<
                    prodNodePath_<<"("<<zookeeper_->getErrorCode()<<")"<<std::endl;
            return false;
        }
        else
        {
            std::cout<<"[SynchroProducer::produce] "<<prodNodePath_<<" : "<<dataPath<<std::endl;
            return true;
        }
    }
    else
    {
        std::cout<<"[SynchroProducer::produce] waiting for ZooKeeper Service ("
                <<zookeeper_->getHosts()<<")"<<std::endl;

        int timewait = 0;
        while (!zookeeper_->isConnected())
        {
            usleep(100000);
            timewait += 100000;
            if (timewait >= 4000000) // xxx
                break;
        }

        if (!zookeeper_->isConnected())
        {
            std::cout<< "Timeout waiting for ZooKeeper !!"<<std::endl;
            return false;
        }
        else
        {
            return doProcude(dataPath);
        }
    }
}

void SynchroProducer::watchConsumers()
{
    //std::cout<<"SynchroProducer::watchConsumers"<<std::endl;
    boost::unique_lock<boost::mutex> lock(consumers_mutex_);

    if (!isSynchronizing_)
        return;

    std::vector<std::string> childrenList;
    zookeeper_->getZNodeChildren(prodNodePath_, childrenList, ZooKeeper::WATCH);

    // todo, consumers
    if (childrenList.size() > 0)
    {
        if (!watchedConsumer_)
            watchedConsumer_ = true; // watched consumer at the first time

        for (size_t i = 0; i < childrenList.size(); i++)
        {
            if (consumersMap_.find(childrenList[i]) == consumersMap_.end())
            {
                consumersMap_[childrenList[i]] = std::make_pair(false, false);
                std::cout<<"[SynchroProducer] watched new consumer "<<childrenList[i]<<std::endl;
            }
        }
    }
}

void SynchroProducer::checkConsumers()
{
    //std::cout<<"SynchroProducer::checkConsumers"<<std::endl;
    boost::unique_lock<boost::mutex> lock(consumers_mutex_);

    if (!isSynchronizing_)
        return;

    //std::cout << consumersMap_.size()<< std::endl;
    consumermap_t::iterator it;
    for (it = consumersMap_.begin(); it != consumersMap_.end(); it++)
    {
        if (it->second.first == true)
        {   // have got result
            continue;
        }

        const std::string& consumer = it->first;
        std::cout<<"[SynchroProducer] checking consumer "<<consumer<<" ";
        std::string data;
        if (zookeeper_->getZNodeData(consumer, data, ZooKeeper::WATCH))
        {
            std::cout<<data<<std::endl;
            if (data == "success")
            {
                it->second.first = true;  // got consumer result
                it->second.second = true; // result is true
            }
            else if (data == "failure")
            {
                it->second.first = true;  // got consumer result
                it->second.second = false; // result is false
            }
            else
                continue;

            consumedCount_ ++; // how many consumers have finished
            zookeeper_->deleteZNode(consumer); // delete node after have got result.
        }
        else
        {
            if (!zookeeper_->isZNodeExists(consumer))
            {
                it->second.first = true;
                it->second.second = false;
                consumedCount_ ++;

                std::cout<<"-- disconnected!!"<<std::endl;
            }
        }
    }

    if (consumedCount_ <= 0)
    {
        return;
    }
    else
    {
        std::cout<<"[SynchroProducer] finished consumer "<<consumedCount_<<", all "<<consumersMap_.size()<<std::endl;
    }

    if (consumedCount_ == consumersMap_.size())
    {
        result_on_consumed_ = true;
        consumermap_t::iterator it;
        for (it = consumersMap_.begin(); it != consumersMap_.end(); it++)
        {
            // xxx, if one of the consumers failed
            if (it->second.second == false)
            {
                result_on_consumed_ = false;
                break;
            }
        }

        if (callback_on_consumed_)
            callback_on_consumed_(result_on_consumed_);

        // Synchronizing finished
        zookeeper_->deleteZNode(prodNodePath_);
        isSynchronizing_ = false;
        std::cout<<"Synchronizing finished"<<std::endl;
    }
}

void SynchroProducer::ensureParentNodes()
{
    // todo
    zookeeper_->createZNode(NodeDef::getSF1RootPath());
    zookeeper_->createZNode(NodeDef::getSynchroPath());
    zookeeper_->createZNode(syncNodePath_);
}

void SynchroProducer::init()
{
    watchedConsumer_ = false;
    consumersMap_.clear();
    consumedCount_ = 0;
    callback_on_consumed_ = NULL;
    result_on_consumed_ = false;
}











