/**
 * @file MasterNodeManager.h
 * @author Zhongxia Li
 * @date Sep 20, 2011S
 * @brief Management of Master node using ZooKeeper.
 */
#ifndef MASTER_NODE_MANAGER_H_
#define MASTER_NODE_MANAGER_H_

#include "NodeDef.h"

#include <map>
#include <vector>
#include <sstream>

#include <3rdparty/zookeeper/ZooKeeper.hpp>
#include <3rdparty/zookeeper/ZooKeeperEvent.hpp>
#include <net/aggregator/AggregatorConfig.h>
#include <util/singleton.h>

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>

using namespace zookeeper;

namespace sf1r
{

class AggregatorManager;

class MasterNodeManager : public ZooKeeperEventHandler
{
public:
    MasterNodeManager();

    void initZooKeeper(const std::string& zkHosts, const int recvTimeout);

    void setNodeInfo(Topology& topology, SF1NodeInfo& sf1NodeInfo);

    /**
     * Register aggregator which requiring info of workers
     * @param aggregator
     */
    void registerAggregator(boost::shared_ptr<AggregatorManager> aggregator)
    {
        aggregatorList_.push_back(aggregator);
    }

    /**
     * Resiger master server after all workers ready
     */
    void startServer();

    /**
     * Whether master is ready
     */
    bool isReady()
    {
        return isReady_;
    }

    net::aggregator::AggregatorConfig& getAggregatorConfig()
    {
        return aggregatorConfig_;
    }

public:
    virtual void process(ZooKeeperEvent& zkEvent);

    virtual void onNodeCreated(const std::string& path);

    virtual void onNodeDeleted(const std::string& path);

    virtual void onDataChanged(const std::string& path);

    /// test
    void showWorkers();

private:
    /**
     * Check whether master node has been ready
     * @return
     */
    bool checkMaster();

    /**
     * Check whether all workers ready (i.e., master it's ready).
     * @return
     */
    bool checkWorkers();

    /**
     * Register SF1 Server atfer master ready.
     */
    void registerServer();

    /**
     * Deregister SF1 server on failure.
     */
    void deregisterServer();

    /***/
    void resetAggregatorConfig();


public:
    struct WorkerState
    {
        bool isRunning_;
        replicaid_t replicaId_;
        nodeid_t nodeId_;
        std::string zkPath_;
        std::string host_;
        unsigned int port_;

        std::string toString()
        {
            std::stringstream ss;
            ss <<"[WorkerState]"<<isRunning_<<" "<<zkPath_<<" "<<host_<<":"<<port_<<endl;
            return ss.str();
        }
    };

    typedef std::map<std::string, boost::shared_ptr<WorkerState> > WorkerStateMapT;

private:
    boost::shared_ptr<ZooKeeper> zookeeper_;

    Topology topology_;
    SF1NodeInfo curNodeInfo_;

    bool isReady_;
    WorkerStateMapT workerStateMap_;

    std::string serverPath_;
    net::aggregator::AggregatorConfig aggregatorConfig_;
    std::vector<boost::shared_ptr<AggregatorManager> > aggregatorList_;

    boost::mutex mutex_;
};

typedef izenelib::util::Singleton<MasterNodeManager> MasterNodeManagerSingleton;

}

#endif /* MASTER_NODE_MANAGER_H_ */
