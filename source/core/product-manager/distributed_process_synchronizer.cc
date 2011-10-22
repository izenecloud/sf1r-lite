#include "distributed_process_synchronizer.h"

#include <node-manager/NodeDef.h>
#include <node-manager/NodeManager.h>

using namespace sf1r;
using namespace zookeeper;

/**

ZooKeeper node definition for this synchronizer:
---
/
|--- SF1R
      |--- Synchro
            |--- ProdManager
                  |--- generate
                  |--- process
---
*/


DistributedProcessSynchronizer::DistributedProcessSynchronizer()
: generated_(false), generated_znode_(false), watched_process_(false), process_result_(false),
  processed_(false), processed_znode_(false)
{
    std::string zkHosts = NodeManagerSingleton::get()->getDSUtilConfig().zkConfig_.zkHosts_;
    int recvTimeout = NodeManagerSingleton::get()->getDSUtilConfig().zkConfig_.zkRecvTimeout_;
    zookeeper_.reset(new ZooKeeper(zkHosts, recvTimeout));
    zookeeper_->registerEventHandler(this);

    // set node paths
    prodNodePath_ = NodeDef::getSynchroPath() + "/ProdManager";
    generateNodePath_ = prodNodePath_ + "/generate";
    processNodePath_ = prodNodePath_ + "/process";

    if (zookeeper_->isConnected())
    {
        initOnConnected();
        isZkConnected_ = true;
    }
    else
    {
        isZkConnected_ = false;
    }
}

bool DistributedProcessSynchronizer::generated(std::string& path)
{
    generated_ = true;
    generatedDataPath_ = path;

    generated_znode_ = false;
    do_generated(generatedDataPath_);

    if (!generated_znode_)
    {
        std::cout<<"[DistributedProcessSynchronizer] generated, waiting for ZooKeeper Service..."<<std::endl;

        int timewait = 0;
        while (!generated_znode_)
        {
            usleep(100000);
            timewait += 100000;
            if (timewait >= 4000000)
                break;
        }

        if (!generated_znode_)
        {
            std::cout << " Timeout !! " <<std::endl;
            return false;
        }
    }

    return true;
}

bool DistributedProcessSynchronizer::processed(bool success)
{
    processed_ = true;
    processedSuccess_ = success;

    processed_znode_ = false;
    do_processed(success);

    if (!processed_znode_)
    {
        std::cout<<"[DistributedProcessSynchronizer] processed, waiting for ZooKeeper Service..."<<std::endl;

        int timewait = 0;
        while (!processed_znode_)
        {
            usleep(100000);
            timewait += 100000;
            if (timewait >= 4000000)
                break;
        }

        if (!processed_znode_)
        {
            std::cout << " Timeout !! " <<std::endl;
            return false;
        }
    }

    return true;
}

void DistributedProcessSynchronizer::watchGenerate(callback_on_generated_t callback_on_generated)
{
    callback_on_generated_ = callback_on_generated;

    processOnGenerated();
}

void DistributedProcessSynchronizer::watchProcess(callback_on_processed_t callback_on_processed)
{
    callback_on_processed_ = callback_on_processed;

    processOnProcessed();
}

void DistributedProcessSynchronizer::joinProcess(bool& result)
{
    result = false;

    int timewait = 0;
    while (!watched_process_)
    {
        usleep(10000);
        timewait += 10000;

        if (timewait >= 4000000)
            break;
    }

    if (watched_process_)
        result = process_result_;
}

/// private

/* virtual */
void DistributedProcessSynchronizer::process(ZooKeeperEvent& zkEvent)
{
    std::cout<<"DistributedProcessSynchronizer::process "<< zkEvent.toString();

    if (!isZkConnected_)
    {
        initOnConnected();
        isZkConnected_ = true;
    }

    if (generated_ && !generated_znode_)
    {
        do_generated(generatedDataPath_);
    }

    if (processed_ && !processed_znode_)
    {
        do_processed(processedSuccess_);
    }
}

/* virtual */
void DistributedProcessSynchronizer::onNodeCreated(const std::string& path)
{
    if (path == generateNodePath_)
    {
        processOnGenerated();
    }

    if (path == processNodePath_)
    {
        processOnProcessed();
    }
}

void DistributedProcessSynchronizer::processOnGenerated()
{
    if (zookeeper_->isZNodeExists(generateNodePath_, ZooKeeper::WATCH))
    {
        std::string generatedDataPath;
        zookeeper_->getZNodeData(generateNodePath_, generatedDataPath);

        if (callback_on_generated_)
            callback_on_generated_(generatedDataPath);
    }
}

void DistributedProcessSynchronizer::processOnProcessed()
{
    if (zookeeper_->isZNodeExists(processNodePath_, ZooKeeper::WATCH))
    {
        std::string data;
        zookeeper_->getZNodeData(processNodePath_, data);

        watched_process_ = true;
        process_result_ = (data=="success" ? true : false);

        if (callback_on_processed_)
            callback_on_processed_( process_result_ );
    }
}

void DistributedProcessSynchronizer::initOnConnected()
{
    zookeeper_->createZNode(NodeDef::getSF1RootPath());
    zookeeper_->createZNode(NodeDef::getSynchroPath());
    zookeeper_->createZNode(prodNodePath_);
}

void DistributedProcessSynchronizer::do_generated(std::string& path)
{
    if (zookeeper_->isConnected())
    {
        if (zookeeper_->createZNode(generateNodePath_, path, ZooKeeper::ZNODE_EPHEMERAL))
        {
            generated_znode_ = true;
        }
        else
        {
            std::cout<<"[DistributedProcessSynchronizer] failed to create generate node: "<<zookeeper_->getErrorCode()<<std::endl;
        }
    }
}

void DistributedProcessSynchronizer::do_processed(bool success)
{
    if (zookeeper_->isConnected())
    {
        std::string data;
        if (success)
            data = "success";
        else
            data = "failed";

        if (zookeeper_->createZNode(processNodePath_, data, ZooKeeper::ZNODE_EPHEMERAL))
        {
            processed_znode_ = false;
        }
        else
        {
            std::cout<<"[DistributedProcessSynchronizer] failed to create process node: "<<zookeeper_->getErrorCode()<<std::endl;
        }
    }
}
