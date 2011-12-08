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
#include <node-manager/SearchNodeManager.h>

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
    static SynchroProducerPtr makeProducer(SynchroType stype, const std::string& id)
    {
        if (syncProducer_.find(id) == syncProducer_.end())
        {
            std::string zkHosts = SearchNodeManager::get()->getDSUtilConfig().zkConfig_.zkHosts_;
            int zkTimeout = SearchNodeManager::get()->getDSUtilConfig().zkConfig_.zkRecvTimeout_;
            std::string syncNodePath = NodeDef::getSynchroPath() + synchroType2ZNode_[stype];
            replicaid_t replicaId = SearchNodeManager::get()->getDSTopologyConfig().curSF1Node_.replicaId_;
            nodeid_t nodeId = SearchNodeManager::get()->getDSTopologyConfig().curSF1Node_.nodeId_;

           syncProducer_[id].reset(new SynchroProducer(zkHosts, zkTimeout, syncNodePath, replicaId, nodeId));
        }

        return syncProducer_[id];
    }

    static SynchroConsumerPtr makeConsumer(SynchroType stype, const std::string& id)
    {
        if (syncConsumer_.find(id) == syncConsumer_.end())
        {
            std::string zkHosts = SearchNodeManager::get()->getDSUtilConfig().zkConfig_.zkHosts_;
            int zkTimeout = SearchNodeManager::get()->getDSUtilConfig().zkConfig_.zkRecvTimeout_;
            std::string syncNodePath = NodeDef::getSynchroPath() + synchroType2ZNode_[stype];
            replicaid_t replicaId = SearchNodeManager::get()->getDSTopologyConfig().curSF1Node_.replicaId_;
            nodeid_t nodeId = SearchNodeManager::get()->getDSTopologyConfig().curSF1Node_.nodeId_;

            syncConsumer_[id].reset(new SynchroConsumer(zkHosts, zkTimeout, syncNodePath, replicaId, nodeId));
        }

        return syncConsumer_[id];
    }

    static void initZKNodes(boost::shared_ptr<ZooKeeper>& zookeeper)
    {
        zookeeper->createZNode(NodeDef::getSynchroPath());

        for (int i = 0; i < SYNCHRO_TYPE_NUM; i++)
        {
            std::string syncModulePath = NodeDef::getSynchroPath() + synchroType2ZNode_[i];
            zookeeper->deleteZNode(syncModulePath, true); // clear children (dirty data)
            zookeeper->createZNode(syncModulePath); // consumers have to watch this node
        }
    }

private:
    static std::string synchroType2ZNode_[SYNCHRO_TYPE_NUM];
    static std::map<std::string, SynchroProducerPtr> syncProducer_;
    static std::map<std::string, SynchroConsumerPtr> syncConsumer_;
};

}

#endif /* DISTRIBUTED_SYNCHRO_FACTORY_H_ */
