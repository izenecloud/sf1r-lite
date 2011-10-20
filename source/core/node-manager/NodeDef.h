/**
 * @file NodeDef.h
 * @author Zhongxia Li
 * @date Sep 20, 2011
 * @brief Consistent definitions of node names and paths used in ZooKeeper for SF1R distributed coordination.
 */
#ifndef NODE_DEF_H_
#define NODE_DEF_H_

#include "TypeDef.h"

#include <sstream>

namespace sf1r {

/**
 *
The topology of distributed SF1 service:
(1) A Mirror is constituted of several SF1 Nodes, it provides full search data and functionality.

(2) Mirrors are replicated, Nodes(Workers) between mirrors should be one-to-one correspondence,
    i.e., "/Mirror1/Node1" and "/Mirror1/Node1" should carry the same search data.
    So that, if "/Mirror1/Node2" broken down, Masters in Mirror1 will know that it can switch to
    worker on "/Mirror2/Node2."

(3) Each node is a SF1 process that can be identified by mirrorId and nodeId (Ids are 1, 2, 3...),
    every node can work as Master or Worker (configurable).

(4) Every Master in all mirriors provides full search functionality in the same manner, they are exposed
    to higher-level applications as replicated SF1 servers.

Using ZooKeeper for distributed coordination, the associated data structure is defined as below:
/
|---
     SF1R-xx
      |--- Topology
             |--- Mirror1
                  |--- Node1
                       |--- Master
                       |--- Worker
                  |--- Node2
                       |--- Worker
             |--- Mirror2
                  |--- Node1
                       |--- Master
                       |--- Worker
                  |--- Node2
                       |--- Master
                       |--- Worker
|--- Service                 # SF1 Servers
     |--- Server00000000     # ---->  "/Topology/Mirror1/Node1/Master"
     |--- Server00000001
     |--- Server00000002
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

    static std::string getMirrorPath(mirrorid_t mirrorId)
    {
        std::stringstream ss;
        ss <<sf1rCluster_<<sf1rTopology_<<"/Mirror"<<mirrorId;

        return ss.str();
    }

    static std::string getNodePath(mirrorid_t mirrorId, nodeid_t nodeId)
    {
        std::stringstream ss;
        ss <<sf1rCluster_<<sf1rTopology_<<"/Mirror"<<mirrorId<<"/Node"<<nodeId;

        return ss.str();
    }

    static std::string getSynchroPath()
    {
        return sf1rCluster_+"/Synchro"; // todo
    }
};

}

#endif /* NODE_DEF_H_ */
