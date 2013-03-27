/**
 * @file ZooKeeperNamespace.h
 * @author Zhongxia Li
 * @date Sep 20, 2011
 * @brief Definition of ZooKeeper namespace for coordination tasks of distributed SF1
 */
#ifndef ZOOKEEPER_NAMESPACE_H_
#define ZOOKEEPER_NAMESPACE_H_

#include "Sf1rTopology.h"

#include <util/kv2string.h>

#include <sstream>

#include <glog/logging.h>

namespace sf1r {

/**
 * ZooKeeper Namespace for SF1
 * @brief Definition of ZooKeeper namespace for coordination tasks of distributed SF1
 *

/                                  # Root of zookeeper namespace
|--- SF1R-[CLUSTERID]              # Root of distributed SF1 namesapce, [CLUSTERID] is specified by user configuration.

      |--- Topology          # Topology of distributed service cluster
           |--- Replica1           # A replica of service cluster
                |--- Node1         # A SF1 node in the replica of cluster, it can be a Master or Worker or both.
                     |--- Search   # A node supply the distributed search service.
                          |--- Master
                          |--- Worker
                     |--- Recommend  # A node supply the distributed Recommend service.
                          |--- Master
                          |--- Worker
                |--- Node2
                     |--- Search
                          |--- Worker
                |--- Node3
                     |--- Recommend
                         |--- Worker
           |--- Replica2
                |--- Node1
                     |--- Search
                          |--- Master
                          |--- Worker
                     |--- Recommend
                          |--- Master
                          |--- Worker
                |--- Node2
                     |--- Search
                          |--- Worker
                |--- Node3
                     |--- Recommend
                         |--- Worker


      |--- Servers           # Each Master service node in topology is a service server. xxx, maybe we can remove this node.
           |--- Server00000000
                     |--- Search, Recommend  # A master node supply Search and Recommend service as master
           |--- Server00000001

      |--- Synchro                 # For synchronization task

 *
 */
class ZooKeeperNamespace
{
    // Define znode names for zookeeper namespace of SF1
    static std::string sf1rCluster_;

    static const std::string primary_;
    static const std::string primaryNodes_;
    static const std::string write_req_queue_;
    static const std::string topology_;
    static const std::string servers_;
    static const std::string replica_;
    static const std::string node_;
    static const std::string server_;

    static const std::string Synchro_;
    static const std::string write_req_prepare_node_;
    static const std::string write_req_seq_;

public:

    static void setClusterId(const std::string& clusterId)
    {
        sf1rCluster_ = "/SF1R-" + clusterId;
    }

    static std::string getSF1RClusterPath()
    {
        return sf1rCluster_;
    }

    static std::string getPrimaryBasePath()
    {
        return sf1rCluster_ + primaryNodes_;
    }
    static std::string getPrimaryNodeParentPath(nodeid_t nodeId)
    {
        std::stringstream ss;
        ss << sf1rCluster_ << primaryNodes_ << node_ << nodeId;
        return ss.str();
    }

    static std::string getPrimaryNodePath(nodeid_t nodeId)
    {
        return getPrimaryNodeParentPath(nodeId) + primary_;
    }

    static std::string getRootWriteReqQueueParent()
    {
        return sf1rCluster_ + write_req_queue_;
    }
    static std::string getCurrWriteReqQueueParent(nodeid_t nodeId)
    {
        std::stringstream ss;
        ss << sf1rCluster_ << write_req_queue_ << node_ << nodeId;
        return ss.str();
    }
    static std::string getWriteReqQueueNode(nodeid_t nodeId)
    {
        std::stringstream ss;
        ss << getCurrWriteReqQueueParent(nodeId) << write_req_seq_;
        return ss.str();
    }
    static std::string getTopologyPath()
    {
        return sf1rCluster_ + topology_;
    }

    static std::string getServerParentPath()
    {
        return sf1rCluster_ + servers_;
    }

    static std::string getServerPath()
    {
        return sf1rCluster_ + servers_ + server_;
    }

    static std::string getReplicaPath(replicaid_t replicaId)
    {
        std::stringstream ss;
        ss <<sf1rCluster_<<topology_<<replica_<<replicaId;

        return ss.str();
    }

    static std::string getNodePath(replicaid_t replicaId, nodeid_t nodeId)
    {
        std::stringstream ss;
        ss <<sf1rCluster_<<topology_<<replica_<<replicaId<<node_<<nodeId;

        return ss.str();
    }

    /// Synchro
    static std::string getSynchroPath()
    {
        return sf1rCluster_ + Synchro_;
    }
    static std::string getWriteReqPrepareNode()
    {
        return sf1rCluster_ + write_req_prepare_node_;
    }
};

/// ZooKeeper Node
class ZNode : public izenelib::util::kv2string
{
public:
    // ZooKeeper Node data key
    const static char* KEY_USERNAME;
    const static char* KEY_HOST;
    const static char* KEY_BA_PORT;
    const static char* KEY_DATA_PORT;
    const static char* KEY_MASTER_NAME;
    const static char* KEY_MASTER_PORT;
    const static char* KEY_WORKER_PORT;
    const static char* KEY_FILESYNC_RPCPORT;
    const static char* KEY_REPLICA_ID;
    const static char* KEY_COLLECTION;
    const static char* KEY_NODE_STATE;
    const static char* KEY_SELF_REG_PRIMARY_PATH;
    const static char* KEY_MASTER_SERVER_REAL_PATH;
    const static char* KEY_PRIMARY_WORKER_REQ_DATA;
    const static char* KEY_REQ_DATA;
    const static char* KEY_REQ_TYPE;
    const static char* KEY_REQ_STEP;
    const static char* KEY_SERVICE_STATE;
    const static char* KEY_SERVICE_NAMES;

    const static char* KEY_FILE;
    const static char* KEY_DIR;
};

}

#endif /* ZOOKEEPER_NAMESPACE_H_ */
