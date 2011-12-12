#include "SynchroProducer.h"

#include <node-manager/NodeDef.h>

#include <sstream>

using namespace sf1r;


SynchroProducer::SynchroProducer(
        boost::shared_ptr<ZooKeeper>& zookeeper,
        const std::string& syncZkNode)
: zookeeper_(zookeeper)
, syncZkNode_(syncZkNode)
, isSynchronizing_(false)
{
    if (zookeeper_)
        zookeeper_->registerEventHandler(this);

    init();
}

SynchroProducer::~SynchroProducer()
{
    zookeeper_->deleteZNode(syncZkNode_, true);
}

bool SynchroProducer::produce(SynchroData& syncData, callback_on_consumed_t callback_on_consumed)
{
    // xxx lock
    if (isSynchronizing_)
    {
        std::cout<<"[SynchroProducer] Re-entry error: synchronizer is working !"<<std::endl;
        return false;
    }
    else
    {
        isSynchronizing_ = true;

        init();
    }

    if (doProcude(syncData))
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

    endSynchroning("synchronize error or timeout!");
    return false;
}

bool SynchroProducer::waitConsumers(bool& isConsumed, int findConsumerTimeout)
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
            endSynchroning("timeout!");
            return false;
        }
    }

    while (isSynchronizing_)
    {
        sleep(1);
    }

    isConsumed = result_on_consumed_;
    return true;
}

/* virtual */
void SynchroProducer::process(ZooKeeperEvent& zkEvent)
{
    //std::cout<<"[SynchroProducer] "<< zkEvent.toString();
}

void SynchroProducer::onNodeDeleted(const std::string& path)
{
    if (consumersMap_.find(path) != consumersMap_.end())
    {
        // check if consumer broken down
        checkConsumers();
    }
}
void SynchroProducer::onDataChanged(const std::string& path)
{
    if (consumersMap_.find(path) != consumersMap_.end())
    {
        // check if consumer finished
        checkConsumers();
    }
}
void SynchroProducer::onChildrenChanged(const std::string& path)
{
    if (path == syncZkNode_)
    {
        // whether new consumer comes, or consumer break down.
        watchConsumers();
        checkConsumers();
    }
}

/// private

bool SynchroProducer::doProcude(SynchroData& syncData)
{
    if (zookeeper_->isConnected())
    {
        ensureParentNodes();

        if (!zookeeper_->createZNode(syncZkNode_, syncData.serialize()))
        {
            if (zookeeper_->getErrorCode() == ZooKeeper::ZERR_ZNODEEXISTS)
            {
                zookeeper_->setZNodeData(syncZkNode_, syncData.serialize());
                std::cout<<"[SynchroProducer] overwrote "<<syncZkNode_<<std::endl;
                //std::cout<<"with data: "<<syncDataStr<<std::endl;
                return true;
            }

            std::cout<<"[SynchroProducer] failed to create "<<
                    syncZkNode_<<" ("<<zookeeper_->getErrorString()<<")"<<std::endl;
            return false;
        }
        else
        {
            std::cout<<"[SynchroProducer] created "<<syncZkNode_<<std::endl;
            //std::cout<<"with data: "<<syncDataStr<<std::endl;
            return true;
        }
    }
    else
    {
        std::cout<<"[SynchroProducer::produce] waiting for ZooKeeper Service ("
                <<zookeeper_->getHosts()<<")"<<std::endl;

        int retryCnt = 0;
        while (!zookeeper_->isConnected())
        {
            zookeeper_->connect(true);

            if ((retryCnt++) > 10)
                break;
        }

        if (!zookeeper_->isConnected())
        {
            std::cout<< "Timeout waiting for ZooKeeper !!"<<std::endl;
            return false;
        }
        else
        {
            return doProcude(syncData);
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
    zookeeper_->getZNodeChildren(syncZkNode_, childrenList, ZooKeeper::WATCH);

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
                std::cout<<"[SynchroProducer] watched a consumer "<<childrenList[i]<<std::endl;
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
        //std::cout<<"[SynchroProducer] checking consumer "<<consumer<<" ";
        std::string sdata;
        if (zookeeper_->getZNodeData(consumer, sdata, ZooKeeper::WATCH))
        {
            //std::cout<<sdata<<std::endl;
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

                std::cout<<"[SynchroProducer]!! lost connection to "<<consumer<<std::endl;
            }
        }
    }

    if (consumedCount_ <= 0)
    {
        return;
    }
    else
    {
        std::cout<<"[SynchroProducer] consumed by "<<consumedCount_<<" consumers, all "<<consumersMap_.size()<<std::endl;
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

        if (callback_on_consumed_)
            callback_on_consumed_(result_on_consumed_);

        // Synchronizing finished
        endSynchroning(info);
    }
}

void SynchroProducer::ensureParentNodes()
{
    //xxx ensure by factory?
    zookeeper_->createZNode(NodeDef::getSF1RootPath());
    zookeeper_->createZNode(NodeDef::getSynchroPath());
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
    zookeeper_->deleteZNode(syncZkNode_);
    isSynchronizing_ = false;
    std::cout<<"Synchronizing finished - "<<info<<std::endl;
}










