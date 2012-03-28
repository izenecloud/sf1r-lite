/**
 * @file RecommendNodeManager.h
 * @author Zhongxia Li
 * @date Dec 8, 2011
 * @brief 
 */
#ifndef RECOMMEND_NODE_MANAGER_H_
#define RECOMMEND_NODE_MANAGER_H_

#include "NodeManagerBase.h"
#include "RecommendMasterManager.h"
#include <aggregator-manager/MasterServerConnector.h>

#include <util/singleton.h>

namespace sf1r
{

class RecommendNodeManager : public NodeManagerBase
{
public:
    RecommendNodeManager()
    {
        CLASSNAME = "RecommendNodeManager";
    }

    static RecommendNodeManager* get()
    {
        return izenelib::util::Singleton<RecommendNodeManager>::get();
    }

protected:
    virtual void setZNodePaths()
    {
        clusterPath_ = ZooKeeperNamespace::getSF1RClusterPath();

        topologyPath_ = ZooKeeperNamespace::getRecommendTopologyPath();
        replicaPath_ = ZooKeeperNamespace::getRecommendReplicaPath(sf1rTopology_.curNode_.replicaId_);
        nodePath_ = ZooKeeperNamespace::getRecommendNodePath(sf1rTopology_.curNode_.replicaId_, sf1rTopology_.curNode_.nodeId_);
    }

    virtual void startMasterManager()
    {
        RecommendMasterManager* recMasterManager = RecommendMasterManager::get();
        recMasterManager->start();

        MasterServerConnector::get()->init(recMasterManager);
    }

    virtual void stopMasterManager()
    {
        RecommendMasterManager::get()->stop();
    }
};

}

#endif /* RECOMMEND_NODE_MANAGER_H_ */
