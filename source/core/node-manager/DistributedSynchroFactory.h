/**
 * @file DistributedSynchroFactory.h
 * @author Zhongxia Li
 * @date Oct 23, 2011
 * @brief
 */
#ifndef DISTRIBUTED_SYNCHRO_FACTORY_H_
#define DISTRIBUTED_SYNCHRO_FACTORY_H_

#include "SynchroProducer.h"
#include "SynchroConsumer.h"

#include <node-manager/NodeDef.h>
#include <node-manager/NodeManager.h>

namespace sf1r{

class DistributedSynchroFactory
{
public:
    enum SynchroType
    {
        // add synchro definition
        SYNCHRO_TYPE_PRODUCT_MANAGER = 0,

        SYNCHRO_TYPE_NUM
    };

public:
    static SynchroProducerPtr makeProcuder(SynchroType stype)
    {
        if (!syncProducer_[stype])
        {
            std::string zkHosts = NodeManagerSingleton::get()->getDSUtilConfig().zkConfig_.zkHosts_;
            int zkTimeout = NodeManagerSingleton::get()->getDSUtilConfig().zkConfig_.zkRecvTimeout_;
            std::string syncNodePath = NodeDef::getSynchroPath() + synchroType2ZNode_[stype];
            replicaid_t replicaId = NodeManagerSingleton::get()->getDSTopologyConfig().curSF1Node_.replicaId_;
            nodeid_t nodeId = NodeManagerSingleton::get()->getDSTopologyConfig().curSF1Node_.nodeId_;

            syncProducer_[stype].reset(new SynchroProducer(zkHosts, zkTimeout, syncNodePath, replicaId, nodeId));
        }

        return syncProducer_[stype];
    }

    static SynchroConsumerPtr makeConsumer(SynchroType stype)
    {
        if (!syncConsumer_[stype])
        {
            std::string zkHosts = NodeManagerSingleton::get()->getDSUtilConfig().zkConfig_.zkHosts_;
            int zkTimeout = NodeManagerSingleton::get()->getDSUtilConfig().zkConfig_.zkRecvTimeout_;
            std::string syncNodePath = NodeDef::getSynchroPath() + synchroType2ZNode_[stype];
            replicaid_t replicaId = NodeManagerSingleton::get()->getDSTopologyConfig().curSF1Node_.replicaId_;
            nodeid_t nodeId = NodeManagerSingleton::get()->getDSTopologyConfig().curSF1Node_.nodeId_;

            syncConsumer_[stype].reset(new SynchroConsumer(zkHosts, zkTimeout, syncNodePath, replicaId, nodeId));
        }

        return syncConsumer_[stype];
    }

private:
    static std::string synchroType2ZNode_[SYNCHRO_TYPE_NUM];
    static SynchroProducerPtr syncProducer_[SYNCHRO_TYPE_NUM];
    static SynchroConsumerPtr syncConsumer_[SYNCHRO_TYPE_NUM];
};

}

#endif /* DISTRIBUTED_SYNCHRO_FACTORY_H_ */
