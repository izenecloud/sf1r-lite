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
: generated_(false), generated_znode_(false), processed_(false), processed_znode_(false)
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

void DistributedProcessSynchronizer::generated(std::string& path)
{
    generated_ = true;
    generatedDataPath_ = path;

    //generated_znode_ = false;
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
    else
    {
        std::cout<<"[DistributedProcessSynchronizer] generated, waiting for ZooKeeper Service..."<<std::endl;
    }
}

void DistributedProcessSynchronizer::processed(bool success)
{
    processed_ = true;
    processedSuccess_ = success;

    //processed_znode_ = false;
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
    else
    {
        std::cout<<"[DistributedProcessSynchronizer] processed, waiting for ZooKeeper Service..."<<std::endl;
    }
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

/// private

/* virtual */
void DistributedProcessSynchronizer::process(ZooKeeperEvent& zkEvent)
{
    std::cout<<"NodeManager::process "<< zkEvent.toString();

    if (!isZkConnected_)
    {
        initOnConnected();
        isZkConnected_ = true;
    }

    if (generated_ && !generated_znode_)
    {
        generated(generatedDataPath_);
    }

    if (processed_ && !processed_znode_)
    {
        processed(processedSuccess_);
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

        callback_on_generated_(generatedDataPath);
    }
}

void DistributedProcessSynchronizer::processOnProcessed()
{
    if (zookeeper_->isZNodeExists(processNodePath_, ZooKeeper::WATCH))
    {
        std::string data;
        zookeeper_->getZNodeData(processNodePath_, data);

        callback_on_processed_( (data=="success" ? true : false) );
    }
}

void DistributedProcessSynchronizer::initOnConnected()
{
    // ensure parent paths
    zookeeper_->createZNode(NodeDef::getSF1RootPath());
    zookeeper_->createZNode(NodeDef::getSynchroPath());
    zookeeper_->createZNode(prodNodePath_);
}
