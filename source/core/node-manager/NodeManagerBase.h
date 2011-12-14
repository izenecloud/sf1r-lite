/**
 * @file NodeManagerBase.h
 * @author Zhongxia Li
 * @date Sep 20, 2011
 * @brief Management of SF1 node using ZooKeeper.
 */
#ifndef NODE_MANAGER_BASE_H_
#define NODE_MANAGER_BASE_H_

#include "NodeDef.h"
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

    ZooKeeperClientPtr zookeeper_;

    // node state
    NodeStateType nodeState_;
    SF1NodeInfo nodeInfo_;
    std::string nodePath_;

    bool masterStarted_;

    boost::mutex mutex_;

    std::string CLASSNAME;
};

}

#endif /* NODE_MANAGER_BASE_H_ */
