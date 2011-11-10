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
    enum NodeStateType
    {
        NODE_STATE_INIT,
        NODE_STATE_STARTING,
        NODE_STATE_STARTING_WAIT_RETRY,
        NODE_STATE_STARTED
    };

    NodeManager();

    ~NodeManager();

    void initWithConfig(
            const DistributedTopologyConfig& dsTopologyConfig,
            const DistributedUtilConfig& dsUtilConfig);

    const DistributedTopologyConfig& getDSTopologyConfig() const
    {
        return dsTopologyConfig_;
    }

    const DistributedUtilConfig& getDSUtilConfig() const
    {
        return dsUtilConfig_;
    }

    const SF1NodeInfo& getNodeInfo() const
    {
        return nodeInfo_;
    }

//    bool isInited()
//    {
//        return inited_;
//    }

public:
    /**
     * Start node manager
     */
    void start();

    /**
     * Stop node manager
     */
    void stop();

//    /**
//     * Register SF1 node for start up. xxx
//     */
//    void registerNode();
//
//    /**
//     * Register Master on current SF1 node. xxx
//     */
//    void registerMaster();
//
//    /**
//     * Register Worker on current SF1 node. xxx
//     */
//    void registerWorker();

    virtual void process(ZooKeeperEvent& zkEvent);

private:
    void initZooKeeper(const std::string& zkHosts, const int recvTimeout);

    /**
     * Make sure zookeeper namaspace (znodes) is initialized properly
     * for all distributed coordination tasks.
     */
    void initZkNameSpace();

    void enterCluster();

    /**
     * Deregister SF1 node on exit
     */
    void leaveCluster();

//    void ensureNodeParents(nodeid_t nodeId, replicaid_t replicaId);
//
//    void initZKNodes();
//
//    void retryRegister();

private:
    DistributedTopologyConfig dsTopologyConfig_;
    DistributedUtilConfig dsUtilConfig_;

//    bool inited_;

    boost::shared_ptr<ZooKeeper> zookeeper_;

    // node state
    NodeStateType nodeState_;

//    bool registercalled_;
//    bool registered_;
    SF1NodeInfo nodeInfo_;
    std::string nodePath_;

//    unsigned int masterPort_;
//    unsigned int workerPort_;

    boost::mutex mutex_;
};


typedef izenelib::util::Singleton<NodeManager> NodeManagerSingleton;

}

#endif /* NODE_MANAGER_H_ */
