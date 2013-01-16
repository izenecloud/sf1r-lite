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
#include <boost/function.hpp>

namespace sf1r
{

class ReqLogMgr;
class NodeManagerBase : public ZooKeeperEventHandler
{
public:
    typedef boost::function<void()> NoFailCBFuncT;
    typedef boost::function<bool()> CanFailCBFuncT;
    typedef boost::function<bool(int, const std::string&)> NewReqCBFuncT;
    enum NodeStateType
    {
        NODE_STATE_INIT,
        NODE_STATE_STARTING,
        NODE_STATE_STARTING_WAIT_RETRY,
        // ready for new write request. that mean last request has finished and
        // the log has written to disk.
        NODE_STATE_STARTED,
        // current node received a new write request
        NODE_STATE_PROCESSING_REQ_RUNNING,
        // primary finished write request and
        // ditribute the request to all replica nodes.
        NODE_STATE_PROCESSING_REQ_NOTIFY_REPLICA_START,
        // primary abort the request and wait all replica to abort the current request.
        NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_ABORT,
        // waiting all replica finish request.
        NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_FINISH_PROCESS,
        // all replica finished request and notify
        // all replica to write request log.
        NODE_STATE_PROCESSING_REQ_WAIT_REPLICA_FINISH_LOG,
        // current node finished the write request and
        // waiting primary notify when all node finished this request.
        NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY,
        // abort a request on non-primary and wait primary to 
        // respond and abort current request to all replica node.
        NODE_STATE_PROCESSING_REQ_WAIT_PRIMARY_ABORT,
        // primary is down, begin new primary election.
        NODE_STATE_ELECTING,
        // restarting after down and begin running the
        // recovery by doing the new requests since last down.
        NODE_STATE_RECOVER_RUNNING,
        // no more new request, waiting primary to finish
        // current request. and wait to sync to latest.
        NODE_STATE_RECOVER_WAIT_PRIMARY,
        // waiting recovery replica to finish sync to latest.
        NODE_STATE_RECOVER_WAIT_REPLICA_FINISH,
        // recovery finished correctly.
        NODE_STATE_RECOVER_FINISH,

        //NODE_STATE_ROLLBACK_RUNNING,
        //NODE_STATE_ROLLBACK_WAIT_PRIMARY,
        //NODE_STATE_ROLLBACK_WAIT_REPLICA_FINISH,

        NODE_STATE_UNKNOWN,
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

    unsigned int getTotalShardNum() const
    {
        return sf1rTopology_.curNode_.master_.totalShardNum_;
    }

    bool isPrimary();


    void beginReqProcess();
    void abortRequest();
    void finishLocalReqProcess(int type, const std::string& reqdata);

    void setCallback(NoFailCBFuncT on_elect_finished, NoFailCBFuncT on_wait_finish_process,
        NoFailCBFuncT on_wait_finish_log, NoFailCBFuncT on_wait_primary, NoFailCBFuncT on_abort_request,
        NoFailCBFuncT on_wait_replica_abort, NewReqCBFuncT on_new_req_from_primary)
    {
        cb_on_elect_finished_ = on_elect_finished;
        cb_on_wait_finish_process_ = on_wait_finish_process;
        cb_on_wait_finish_log_ = on_wait_finish_log;
        cb_on_wait_primary_ = on_wait_primary;
        cb_on_abort_request_ = on_abort_request;
        cb_on_wait_replica_abort_ = on_wait_replica_abort;
        cb_on_new_req_from_primary_ = on_new_req_from_primary;
    }

    void setRecoveryCallback(NoFailCBFuncT on_recovering, NoFailCBFuncT on_recover_wait_primary,
        NoFailCBFuncT on_recover_wait_replica_finish)
    {
        cb_on_recovering_ = on_recovering;
        cb_on_recover_wait_primary_ = on_recover_wait_primary;
        cb_on_recover_wait_replica_finish_ = on_recover_wait_replica_finish;
    }

    boost::shared_ptr<ReqLogMgr> getReqLogMgr()
    {
        return reqlog_mgr_;
    }

public:
    virtual void process(ZooKeeperEvent& zkEvent);
    virtual void onNodeDeleted(const std::string& path);
    virtual void onDataChanged(const std::string& path);
    virtual void onChildrenChanged(const std::string& path);

protected:
    virtual void setZNodePaths() = 0;

    virtual void setMasterDistributeState(bool enable) = 0;

    virtual void startMasterManager() {}

    virtual void stopMasterManager() {}

    virtual void detectMasters() {}

    bool isPrimaryWithoutLock() const;

    void setNodeState(NodeStateType state);
protected:
    /**
     * Make sure Zookeeper namespace is initialized properly
     */
    void tryInitZkNameSpace();

    bool checkZooKeeperService();

    void setSf1rNodeData(ZNode& znode);

    void enterCluster(bool start_master = true);
    void enterClusterAfterRecovery(bool start_master = true);

    void unregisterPrimary();
    void registerPrimary(ZNode& znode);
    void updateCurrentPrimary();
    void updateNodeState();
    NodeStateType getPrimaryState();
    NodeStateType getNodeState(const std::string& nodepath);
    void checkSecondaryState();
    void checkSecondaryElecting();
    void checkSecondaryReqProcess();
    void checkSecondaryReqFinishLog();
    void checkSecondaryReqAbort();
    void checkSecondaryRecovery();
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
    std::string primaryBasePath_;
    std::string primaryNodeParentPath_;
    std::string primaryNodePath_;

    boost::mutex mutex_;

    std::string CLASSNAME;
    std::string self_primary_path_;
    std::string curr_primary_path_;
    NoFailCBFuncT cb_on_elect_finished_;
    NoFailCBFuncT cb_on_wait_finish_process_;
    NoFailCBFuncT cb_on_wait_finish_log_;
    NoFailCBFuncT cb_on_wait_primary_;
    NoFailCBFuncT cb_on_abort_request_;
    NoFailCBFuncT cb_on_wait_replica_abort_;
    NoFailCBFuncT cb_on_recovering_;
    NoFailCBFuncT cb_on_recover_wait_primary_;
    NoFailCBFuncT cb_on_recover_wait_replica_finish_;
    NewReqCBFuncT cb_on_new_req_from_primary_;
    boost::shared_ptr<ReqLogMgr> reqlog_mgr_;
    //typedef std::map<std::string, NodeStateType> ElectingNodeMapT;
    //ElectingNodeMapT electing_secondaries_;
};

}

#endif /* NODE_MANAGER_BASE_H_ */
