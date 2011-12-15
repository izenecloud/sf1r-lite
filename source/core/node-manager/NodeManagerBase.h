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
    void init(const DistributedTopologyConfig& dsTopologyConfig);

    /**
     * Start node manager
     */
    void start();

    /**
     * Stop node manager
     */
    void stop();

    const DistributedTopologyConfig& getDSTopologyConfig() const
    {
        return dsTopologyConfig_;
    }

    const SF1NodeInfo& getNodeInfo() const
    {
        return nodeInfo_;
    }

    unsigned int getShardNum() const
    {
        return dsTopologyConfig_.shardNum_;
    }

public:
    virtual void process(ZooKeeperEvent& zkEvent);

protected:
    virtual void setZNodePaths() = 0;

    virtual void startMasterManager() {}

    virtual void stopMasterManager() {}

protected:
    /**
     * Make sure zookeeper namaspace (znodes) is initialized properly
     */
    void initZkNameSpace();

    void enterCluster();

    /**
     * Deregister SF1 node on exit
     */
    void leaveCluster();

protected:
    DistributedTopologyConfig dsTopologyConfig_;
    SF1NodeInfo nodeInfo_;

    NodeStateType nodeState_;
    bool masterStarted_;

    ZooKeeperClientPtr zookeeper_;
    // znode paths (from root) corresponding current sf1 node
    std::string clusterPath_;
    std::string topologyPath_;
    std::string replicaPath_;
    std::string nodePath_;

    boost::mutex mutex_;

    std::string CLASSNAME;
};

}

#endif /* NODE_MANAGER_BASE_H_ */
