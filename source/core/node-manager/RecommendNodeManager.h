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

#include <util/singleton.h>

namespace sf1r
{

class RecommendNodeManager : public NodeManagerBase
{
public:
    RecommendNodeManager()
    {
        CLASSNAME = "[RecommendNodeManager]";
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
        replicaPath_ = ZooKeeperNamespace::getRecommendReplicaPath(nodeInfo_.replicaId_);
        nodePath_ = ZooKeeperNamespace::getRecommendNodePath(nodeInfo_.replicaId_, nodeInfo_.nodeId_);
    }

    virtual void startMasterManager()
    {
        RecommendMasterManager::get()->start();
    }

    virtual void stopMasterManager()
    {
        RecommendMasterManager::get()->stop();
    }
};

}

#endif /* RECOMMEND_NODE_MANAGER_H_ */
