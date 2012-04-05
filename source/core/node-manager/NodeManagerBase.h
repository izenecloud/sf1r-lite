/**
 * @file NodeManagerBase.h
 * @author Zhongxia Li
 * @date Sep 20, 2011
 * @brief Management of SF1 node using ZooKeeper.
 */
#ifndef NODE_MANAGER_BASE_H_
#define NODE_MANAGER_BASE_H_

#include "ZooKeeperNamespace.h"
#include "ZooKeeperManager.h"

#include <configuration-manager/DistributedTopologyConfig.h>
#include <configuration-manager/DistributedUtilConfig.h>

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

namespace sf1r
{

class NodeManagerBase : public ZooKeeperEventHandler
{
public:
    enum NodeStateType
    {
        NODE_STATE_INIT,
        NODE_STATE_STARTING,
        NODE_STATE_STARTING_WAIT_RETRY,
        NODE_STATE_STARTED
    };

    NodeManagerBase();

    virtual ~NodeManagerBase();

    /**
     * @param dsTopologyConfig
     */
    void init(const DistributedTopologyConfig& distributedTopologyConfig);

    /**
     * Start node manager
     */
    void start();

    /**
     * Stop node manager
     */
    void stop();

    const Sf1rTopology& getSf1rTopology() const
    {
        return sf1rTopology_;
    }

    const Sf1rNode& getCurrentSf1rNode() const
    {
        return sf1rTopology_.curNode_;
    }

    unsigned int getShardNum() const
    {
        return sf1rTopology_.curNode_.master_.totalShardNum_;
    }

public:
    virtual void process(ZooKeeperEvent& zkEvent);

protected:
    virtual void setZNodePaths() = 0;

    virtual void startMasterManager() {}

    virtual void stopMasterManager() {}

protected:
    /**
     * Make sure Zookeeper namespace is initialized properly
     */
    void tryInitZkNameSpace();

    bool checkZooKeeperService();

    void setSf1rNodeData(ZNode& znode);

    void enterCluster();

    /**
     * Deregister SF1 node on exit
     */
    void leaveCluster();

protected:
    bool isDistributionEnabled_;
    Sf1rTopology sf1rTopology_;

    NodeStateType nodeState_;
    bool masterStarted_;

    ZooKeeperClientPtr zookeeper_;

    std::string clusterPath_;
    std::string topologyPath_;
    std::string replicaPath_;
    std::string nodePath_;

    boost::mutex mutex_;

    std::string CLASSNAME;
};

}

#endif /* NODE_MANAGER_BASE_H_ */
