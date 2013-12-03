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

#include <node-manager/ZooKeeperNamespace.h>
#include <node-manager/ZooKeeperManager.h>
//#include <node-manager/NodeManagerBase.h>

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
            ZooKeeperClientPtr zkClient = ZooKeeperManager::get()->createClient(NULL, true);
            syncProducerMap_[syncID].reset(new SynchroProducer(zkClient, syncID));
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
        ZooKeeperClientPtr zkClient = ZooKeeperManager::get()->createClient(NULL, true);
        SynchroConsumerPtr ret(new SynchroConsumer(zkClient, syncID));
        return ret;
    }

private:
    static std::map<std::string, SynchroProducerPtr> syncProducerMap_;
};

}

#endif /* DISTRIBUTED_SYNCHRO_FACTORY_H_ */
