/**
 * @file NodeManager.h
 * @author Zhongxia Li
 * @date Sep 20, 2011
 * @brief Management of SF1 node using ZooKeeper.
 */
#ifndef NODE_MANAGER_H_
#define NODE_MANAGER_H_

#include "NodeDef.h"

#include <3rdparty/zookeeper/ZooKeeper.hpp>
#include <3rdparty/zookeeper/ZooKeeperEvent.hpp>
#include <util/singleton.h>

#include <configuration-manager/DistributedTopologyConfig.h>
#include <configuration-manager/DistributedUtilConfig.h>

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

using namespace zookeeper;

namespace sf1r
{

class NodeManager : public ZooKeeperEventHandler
{
public:
    NodeManager();

    ~NodeManager();

    void initWithConfig(
            const DistributedTopologyConfig& dsTopologyConfig,
            const DistributedUtilConfig& dsUtilConfig)
    {
        dsTopologyConfig_ = dsTopologyConfig;
        dsUtilConfig_ = dsUtilConfig;

        NodeDef::setClusterIdNodeName(dsTopologyConfig_.clusterId_);

        initZooKeeper(dsUtilConfig_.zkConfig_.zkHosts_, dsUtilConfig_.zkConfig_.zkRecvTimeout_);
        initTopology();
    }

    const DistributedTopologyConfig& getDSTopologyConfig() const
    {
        return dsTopologyConfig_;
    }

    const DistributedUtilConfig& getDSUtilConfig() const
    {
        return dsUtilConfig_;
    }

public:
    void initZooKeeper(const std::string& zkHosts, const int recvTimeout);

    void setCurrentNodeInfo(SF1NodeInfo& sf1NodeInfo);

    /**
     * Register SF1 node for start up.
     */
    void registerNode();

    /**
     * Register Master on current SF1 node.
     */
    void registerMaster(unsigned int port);

    /**
     * Register Worker on current SF1 node.
     */
    void registerWorker(unsigned int port);

    /**
     * Deregister SF1 node on exit
     */
    void deregisterNode();

    virtual void process(ZooKeeperEvent& zkEvent);

private:
    void ensureNodeParents(nodeid_t nodeId, replicaid_t replicaId);

    void initTopology();

    void retryRegister();

private:
    DistributedTopologyConfig dsTopologyConfig_;
    DistributedUtilConfig dsUtilConfig_;

    boost::shared_ptr<ZooKeeper> zookeeper_;

    bool registercalled_;
    bool registered_;
    SF1NodeInfo nodeInfo_;
    std::string nodePath_;

    unsigned int masterPort_;
    unsigned int workerPort_;

    boost::mutex mutex_;
};


typedef izenelib::util::Singleton<NodeManager> NodeManagerSingleton;

}

#endif /* NODE_MANAGER_H_ */
