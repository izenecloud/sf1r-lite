/**
 * @file SynchroFactory.h
 * @author Zhongxia Li
 * @date Oct 23, 2011
 * @brief
 */
#ifndef DISTRIBUTED_SYNCHRO_FACTORY_H_
#define DISTRIBUTED_SYNCHRO_FACTORY_H_

#include "SynchroProducer.h"
#include "SynchroConsumer.h"

#include <node-manager/NodeDef.h>
#include <node-manager/ZooKeeperManager.h>
#include <node-manager/SearchNodeManager.h>

namespace sf1r{

/**
 * A synchro task works in \b one-producer-multiple-consumer(s) pattern;
 * Each synchro task is identified by a unique synchro ID in string type (given by user).
 */
class SynchroFactory
{
public:
    /**
     * Each synchro ID corresponding to one Producer.
     * @param syncID
     * @return SynchroProducer Ptr
     */
    static SynchroProducerPtr getProducer(const std::string& syncID)
    {
        if (syncProducerMap_.find(syncID) == syncProducerMap_.end())
        {
            std::string zkHosts = SearchNodeManager::get()->getDSUtilConfig().zkConfig_.zkHosts_;
            int zkTimeout = SearchNodeManager::get()->getDSUtilConfig().zkConfig_.zkRecvTimeout_;
            std::string syncZkNode = NodeDef::getSynchroPath() + "/" + syncID;

            syncProducerMap_[syncID].reset(new SynchroProducer(zkHosts, zkTimeout, syncZkNode));
        }

        return syncProducerMap_[syncID];
    }

    /**
     * Each synchro ID corresponding to multiple Consumers.
     * @param syncID
     * @return SynchroConsumer Ptr
     */
    static SynchroConsumerPtr getConsumer(const std::string& syncID)
    {
        std::string zkHosts = SearchNodeManager::get()->getDSUtilConfig().zkConfig_.zkHosts_;
        int zkTimeout = SearchNodeManager::get()->getDSUtilConfig().zkConfig_.zkRecvTimeout_;
        std::string syncZkNode = NodeDef::getSynchroPath() + "/" + syncID;

        SynchroConsumerPtr ret(new SynchroConsumer(zkHosts, zkTimeout, syncZkNode));
        return ret;
    }

    static void initSynchroNode(ZooKeeperClientPtr& zookeeper)
    {
        zookeeper->deleteZNode(NodeDef::getSynchroPath(), true);
        zookeeper->createZNode(NodeDef::getSynchroPath());
    }

private:
    static std::map<std::string, SynchroProducerPtr> syncProducerMap_;
};

}

#endif /* DISTRIBUTED_SYNCHRO_FACTORY_H_ */
