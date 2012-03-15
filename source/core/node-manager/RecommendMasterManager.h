/**
 * @file RecommendMasterManager.h
 * @author Zhongxia Li
 * @date Dec 8, 2011
 * @brief 
 */
#ifndef RECOMMEND_MASTER_MANAGER_H_
#define RECOMMEND_MASTER_MANAGER_H_

#include "MasterManagerBase.h"
#include "SearchMasterAddressFinder.h"

#include <util/singleton.h>

namespace sf1r
{

class RecommendMasterManager : public MasterManagerBase, public SearchMasterAddressFinder
{
public:
    RecommendMasterManager();

    static RecommendMasterManager* get()
    {
        return izenelib::util::Singleton<RecommendMasterManager>::get();
    }

    virtual bool init();

    virtual bool findSearchMasterAddress(std::string& host, uint32_t& port);

protected:
    virtual std::string getReplicaPath(replicaid_t replicaId)
    {
        return ZooKeeperNamespace::getRecommendReplicaPath(replicaId);
    }

    virtual std::string getNodePath(replicaid_t replicaId, nodeid_t nodeId)
    {
        return ZooKeeperNamespace::getRecommendNodePath(replicaId, nodeId);
    }
};

}

#endif /* RECOMMEND_MASTER_MANAGER_H_ */
