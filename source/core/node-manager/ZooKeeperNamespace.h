/**
 * @file ZooKeeperNamespace.h
 * @author Zhongxia Li
 * @date Sep 20, 2011
 * @brief Definition of ZooKeeper namespace for coordination tasks of distributed SF1
 */
#ifndef ZOOKEEPER_NAMESPACE_H_
#define ZOOKEEPER_NAMESPACE_H_

#include "NodeTypeDef.h"
#include <util/kv2string.h>
#include <sstream>

namespace sf1r {

/**
 * ZooKeeper Namespace for SF1
 * @brief Definition of ZooKeeper namespace for coordination tasks of distributed SF1
 *

/                                  # Root of zookeeper namespace
|--- SF1R-[CLUSTERID]              # Root of distributed SF1 namesapce, [CLUSTERID] is specified by user configuration.

      |--- SearchTopology          # Topology of distributed search cluster
           |--- Replica1           # A replica of search cluster
                |--- Node1         # A SF1 node in the replica of search cluster, it can be a Master or Worker or both.
                     |--- Master
                     |--- Worker
                |--- Node2
                     |--- Worker
                |--- ...
           |--- Replica2
                |--- Node1
                     |--- Master
                     |--- Worker
                |--- Node2
                     |--- Worker
                |--- ...

      |--- SearchServers           # Each Master node in search topology is a search server
           |--- Server00000000
           |--- Server00000001
           |--- ...

      |--- RecommendTopology       # Topology of distributed recommend cluster, similar to Search.
           |--- Replica1
           |--- Replica2

      |--- RecommendServers        # Each Master node in recommend topology is a recommend server
           |--- Server00000000
           |--- Server00000001
           |--- ...

      |--- Synchro                 # For synchronization task

 *
 */
class ZooKeeperNamespace
{
    // Define znode names for zookeeper namespace of SF1
    static std::string sf1rCluster_;

    static const std::string searchTopology_;
    static const std::string searchServers_;
    static const std::string recommendTopology_;
    static const std::string recommendServers_;
    static const std::string replica_;
    static const std::string node_;
    static const std::string server_;

    static const std::string Synchro_;

public:

    static void setClusterId(const std::string& clusterId)
    {
        sf1rCluster_ = "/SF1R-" + clusterId;
    }

    static std::string getSF1RClusterPath()
    {
        return sf1rCluster_;
    }

    /// Search
    static std::string getSearchTopologyPath()
    {
        return sf1rCluster_ + searchTopology_;
    }

    static std::string getSearchServerParentPath()
    {
        return sf1rCluster_ + searchServers_;
    }

    static std::string getSearchServerPath()
    {
        return sf1rCluster_ + searchServers_ + server_;
    }

    static std::string getSearchReplicaPath(replicaid_t replicaId)
    {
        std::stringstream ss;
        ss <<sf1rCluster_<<searchTopology_<<replica_<<replicaId;

        return ss.str();
    }

    static std::string getSearchNodePath(replicaid_t replicaId, nodeid_t nodeId)
    {
        std::stringstream ss;
        ss <<sf1rCluster_<<searchTopology_<<replica_<<replicaId<<node_<<nodeId;

        return ss.str();
    }

    /// Recommend
    static std::string getRecommendTopologyPath()
    {
        return sf1rCluster_ + recommendTopology_;
    }

    static std::string getRecommendServerParentPath()
    {
        return sf1rCluster_ + recommendServers_;
    }

    static std::string getRecommendServerPath()
    {
        return sf1rCluster_ + recommendServers_ + server_;
    }

    static std::string getRecommendReplicaPath(replicaid_t replicaId)
    {
        std::stringstream ss;
        ss <<sf1rCluster_<<recommendTopology_<<replica_<<replicaId;

        return ss.str();
    }

    static std::string getRecommendNodePath(replicaid_t replicaId, nodeid_t nodeId)
    {
        std::stringstream ss;
        ss <<sf1rCluster_<<recommendTopology_<<replica_<<replicaId<<node_<<nodeId;

        return ss.str();
    }

    /// Synchro
    static std::string getSynchroPath()
    {
        return sf1rCluster_ + Synchro_;
    }
};

/// ZooKeeper Node
class ZNode : public izenelib::util::kv2string
{
public:
    // ZooKeeper Node data key
    const static char* KEY_HOST;
    const static char* KEY_BA_PORT;
    const static char* KEY_DATA_PORT;
    const static char* KEY_NOTIFY_PORT;
    const static char* KEY_WORKER_PORT;
    const static char* KEY_SHARD_ID;
    const static char* KEY_COLLECTION;

    const static char* KEY_FILE;
    const static char* KEY_DIR;
};

}

#endif /* ZOOKEEPER_NAMESPACE_H_ */
