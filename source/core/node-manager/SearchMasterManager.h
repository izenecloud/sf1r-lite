/**
 * @file SearchMasterManager.h
 * @author Zhongxia Li
 * @date Dec 8, 2011
 * @brief 
 */
#ifndef SEARCH_MASTER_MANAGER_H_
#define SEARCH_MASTER_MANAGER_H_

#include "MasterManagerBase.h"

#include <util/singleton.h>

namespace sf1r
{

class SearchMasterManager : public MasterManagerBase
{
public:
    SearchMasterManager();

    static SearchMasterManager* get()
    {
        return izenelib::util::Singleton<SearchMasterManager>::get();
    }

    virtual bool init();

protected:
    virtual std::string getReplicaPath(replicaid_t replicaId)
    {
        return ZooKeeperNamespace::getSearchReplicaPath(replicaId);
    }

    virtual std::string getNodePath(replicaid_t replicaId, nodeid_t nodeId)
    {
        return ZooKeeperNamespace::getSearchNodePath(replicaId, nodeId);
    }

    virtual std::string getPrimaryNodeParentPath(nodeid_t nodeId)
    {
        return ZooKeeperNamespace::getSearchPrimaryNodeParentPath(nodeId);
    }


};


}

#endif /* SEARCH_MASTER_MANAGER_H_ */
