/**
 * @file MasterManagerBase.h
 * @author Zhongxia Li
 * @date Sep 20, 2011S
 * @brief Management (Coordination) for Master node using ZooKeeper.
 */
#ifndef MASTER_MANAGER_BASE_H_
#define MASTER_MANAGER_BASE_H_

#include "ZooKeeperNamespace.h"
#include "ZooKeeperManager.h"

#include <map>
#include <vector>
#include <sstream>

#include <net/aggregator/AggregatorConfig.h>
#include <net/aggregator/Aggregator.h>

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

using namespace net::aggregator;

namespace sf1r
{

class MasterManagerBase : public ZooKeeperEventHandler
{
public:
    enum MasterStateType
    {
        MASTER_STATE_INIT,
        MASTER_STATE_STARTING,
        MASTER_STATE_STARTING_WAIT_ZOOKEEPER,
        MASTER_STATE_STARTING_WAIT_WORKERS,
        MASTER_STATE_STARTED,
        MASTER_STATE_FAILOVERING,
        MASTER_STATE_RECOVERING
    };

    typedef std::map<shardid_t, boost::shared_ptr<Sf1rNode> > WorkerMapT;

public:
    MasterManagerBase();

    virtual ~MasterManagerBase() { stop(); };

    virtual bool init() = 0;

    void start();

    void stop();

    /**
     * Register aggregators
     * @param aggregator
     */
    void registerAggregator(boost::shared_ptr<AggregatorBase> aggregator)
    {
        aggregatorList_.push_back(aggregator);
    }

    /**
     * Get data receiver address by shard id.
     * @param shardid   [IN]
     * @param host      [OUT]
     * @param recvPort  [OUT]
     * @return true on success, false on failure.
     */
    bool getShardReceiver(
            unsigned int shardid,
            std::string& host,
            unsigned int& recvPort);

public:
    virtual void process(ZooKeeperEvent& zkEvent);

    virtual void onNodeCreated(const std::string& path);

    virtual void onNodeDeleted(const std::string& path);

    virtual void onChildrenChanged(const std::string& path);

    /// test
    void showWorkers();

protected:
    virtual std::string getReplicaPath(replicaid_t replicaId) = 0;

    virtual std::string getNodePath(replicaid_t replicaId, nodeid_t nodeId) = 0;

protected:
    std::string state2string(MasterStateType e);

    void watchAll();

    bool checkZooKeeperService();

    void doStart();

protected:
    int detectWorkers();

    void updateWorkerNode(boost::shared_ptr<Sf1rNode>& workerNode, ZNode& znode);

    void detectReplicaSet(const std::string& zpath="");

    /**
     * If any node in current cluster replica is broken down,
     * switch to another node in its replica set.
     */
    void failover(const std::string& zpath);

    bool failover(boost::shared_ptr<Sf1rNode>& sf1rNode);

    /**
     * Recover after nodes in current cluster replica came back from failure.
     */
    void recover(const std::string& zpath);

    /**
     * Register SF1 Server atfer master ready.
     */
    void registerSearchServer();

    /**
     * Deregister SF1 server on failure.
     */
    void deregisterSearchServer();

    /***/
    void resetAggregatorConfig();

protected:
    Sf1rTopology sf1rTopology_;

    ZooKeeperClientPtr zookeeper_;

    // znode paths
    std::string topologyPath_;
    std::string serverParentPath_;
    std::string serverPath_;
    std::string serverRealPath_;

    MasterStateType masterState_;
    boost::mutex state_mutex_;

    std::vector<replicaid_t> replicaIdList_;
    boost::mutex replica_mutex_;

    WorkerMapT workerMap_;
    boost::mutex workers_mutex_;

    std::vector<boost::shared_ptr<AggregatorBase> > aggregatorList_;

    std::string CLASSNAME;
};



}

#endif /* MASTER_MANAGER_BASE_H_ */
