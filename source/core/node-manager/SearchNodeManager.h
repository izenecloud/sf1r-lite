/**
 * @file SearchNodeManager.h
 * @author Zhongxia Li
 * @date Dec 8, 2011
 * @brief 
 */
#ifndef SEARCH_NODE_MANAGER_H_
#define SEARCH_NODE_MANAGER_H_

#include "NodeManagerBase.h"
#include "SearchMasterManager.h"

#include <aggregator-manager/MasterNotifier.h>

#include <util/singleton.h>

#include <boost/lexical_cast.hpp>

namespace sf1r
{

class SearchNodeManager : public NodeManagerBase
{
public:
    SearchNodeManager()
    {
        CLASSNAME = "SearchNodeManager";
    }

    static SearchNodeManager* get()
    {
        return izenelib::util::Singleton<SearchNodeManager>::get();
    }

protected:
    virtual void setZNodePaths()
    {
        clusterPath_ = ZooKeeperNamespace::getSF1RClusterPath();

        topologyPath_ = ZooKeeperNamespace::getSearchTopologyPath();
        replicaPath_ = ZooKeeperNamespace::getSearchReplicaPath(sf1rTopology_.curNode_.replicaId_);
        nodePath_ = ZooKeeperNamespace::getSearchNodePath(sf1rTopology_.curNode_.replicaId_, sf1rTopology_.curNode_.nodeId_);
    }

    virtual void startMasterManager()
    {
        SearchMasterManager::get()->start();
    }

    virtual void stopMasterManager()
    {
        SearchMasterManager::get()->stop();
    }

    virtual void detectMasters()
    {
        boost::lock_guard<boost::mutex> lock(MasterNotifier::get()->getMutex());
        MasterNotifier::get()->clear();

        replicaid_t replicaId = sf1rTopology_.curNode_.replicaId_;
        std::vector<std::string> childrenList;
        zookeeper_->getZNodeChildren(ZooKeeperNamespace::getSearchReplicaPath(replicaId), childrenList, ZooKeeper::WATCH); // set watcher
        for (nodeid_t nodeId = 1; nodeId <= sf1rTopology_.nodeNum_; nodeId++)
        {
            std::string data;
            std::string nodePath = ZooKeeperNamespace::getSearchNodePath(replicaId, nodeId);
            if (zookeeper_->getZNodeData(nodePath, data, ZooKeeper::WATCH))
            {
                ZNode znode;
                znode.loadKvString(data);

                // if this sf1r node provides master server
                if (znode.hasKey(ZNode::KEY_MASTER_PORT))
                {
                    std::string masterHost = znode.getStrValue(ZNode::KEY_HOST);
                    uint32_t masterPort;
                    try
                    {
                        masterPort = boost::lexical_cast<uint32_t>(znode.getStrValue(ZNode::KEY_MASTER_PORT));
                    }
                    catch (std::exception& e)
                    {
                        LOG (ERROR) << "failed to convert masterPort \"" << znode.getStrValue(ZNode::KEY_BA_PORT)
                                    << "\" got from master on node" << nodeId << "@" << masterHost;
                        continue;
                    }

                    LOG (INFO) << "detected Master " << masterHost << ":" << masterPort;
                    MasterNotifier::get()->addMasterAddress(masterHost, masterPort);
                }
            }
        }
    }

};


}

#endif /* SEARCH_NODE_MANAGER_H_ */
