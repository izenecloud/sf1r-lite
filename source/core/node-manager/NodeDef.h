/**
 * @file NodeDef.h
 * @author Zhongxia Li
 * @date Sep 20, 2011
 * @brief
 */
#ifndef NODE_DEF_H_
#define NODE_DEF_H_

#include "TypeDef.h"
#include <util/kv2string.h>
#include <sstream>

namespace sf1r {

/**
 * @brief Consistent definitions of znodes used in ZooKeeper for SF1R distributed coordination.
 *
The topology of distributed SF1 service:
(1) A distribution of SF1 is constituted of several SF1 Nodes, which provides full search data and functionality.

(2) The distribution of SF1 can be replicated, Nodes(Workers) between replica should be one-to-one correspondence,
    i.e., "/Replica1/Node1" and "/Replica2/Node1" should carry the same search data, so that, if "/Replica1/Node2"
    is broken down, Masters in Replica1 will know that it can switch to worker on "/Replica2/Node2."

(3) Each node is a SF1 process that can be identified by replicaId and nodeId (Ids are 1, 2, 3...),
    every node can work as Master or Worker (configurable).

(4) Every Master in all replicas provides full search functionality in the same manner, they are exposed
    to higher-level applications as replicated SF1 servers.

Using ZooKeeper for distributed coordination, the associated data structure is defined as below:
/
|--- SF1R-xx
      |--- Topology
             |--- Replica1
                  |--- Node1
                       |--- Master
                       |--- Worker
                  |--- Node2
                       |--- Worker
             |--- Replica2
                  |--- Node1
                       |--- Worker
                  |--- Node2
                       |--- Worker
                       |--- Master
      |--- Service
             |--- Server00000000     # ---->  "/Topology/Replica1/Node1/Master"
             |--- Server00000001     # ---->  "/Topology/Replica2/Node2/Master"
      |--- Synchro
 *
 */
class NodeDef
{
    // ZooKeeper node names' definitions
    static std::string sf1rCluster_;         // identify SF1R application, configured
    static const std::string sf1rTopology_;
    static const std::string sf1rService_;

public:

    static void setClusterIdNodeName(const std::string& clusterId)
    {
        sf1rCluster_ = "/SF1R-" + clusterId;
    }

    static std::string getSF1RootPath()
    {
        return sf1rCluster_;
    }

    static std::string getSF1TopologyPath()
    {
        return sf1rCluster_ + sf1rTopology_;
    }

    static std::string getSF1ServicePath()
    {
        return sf1rCluster_ + sf1rService_;
    }

    static std::string getReplicaPath(replicaid_t replicaId)
    {
        std::stringstream ss;
        ss <<sf1rCluster_<<sf1rTopology_<<"/Replica"<<replicaId;

        return ss.str();
    }

    static std::string getNodePath(replicaid_t replicaId, nodeid_t nodeId)
    {
        std::stringstream ss;
        ss <<sf1rCluster_<<sf1rTopology_<<"/Replica"<<replicaId<<"/Node"<<nodeId;

        return ss.str();
    }

    static std::string getSynchroPath()
    {
        return sf1rCluster_+"/Synchro"; // todo
    }
};

class NodeData : public izenelib::util::kv2string
{
public:
    // topology
    const static char* NDATA_KEY_HOST;
    const static char* NDATA_KEY_BA_PORT;
    const static char* NDATA_KEY_DATA_PORT;
    const static char* NDATA_KEY_NOTIFY_PORT;
    const static char* NDATA_KEY_WORKER_PORT;
    const static char* NDATA_KEY_SHARD_ID;

    // synchronizer
    const static char* ND_KEY_SYNC_PRODUCER;
    const static char* ND_KEY_SYNC_CONSUMER;

    // data
    const static char* ND_KEY_FILE;
    const static char* ND_KEY_DIR;

};

}

#endif /* NODE_DEF_H_ */
