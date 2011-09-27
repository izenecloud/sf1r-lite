/**
 * @file NodeDef.h
 * @author Zhongxia Li
 * @date Sep 20, 2011
 * @brief 
 */
#ifndef NODE_DEF_H_
#define NODE_DEF_H_

#include <stdint.h>
#include <string>
#include <map>
#include <sstream>

namespace sf1r {

/// Definitions
typedef uint32_t nodeid_t;
typedef uint32_t mirrorid_t;


const static char ZK_PATH_SF1[]          = "/SF1Root";
const static char ZK_PATH_SF1_TOPOLOGY[] = "/SF1Root/Topology";
const static char ZK_PATH_SF1_SERVICE[]  = "/SF1Root/Service";

/**
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
|
SF1Root
|--- Topology                #
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
**/
struct Topology
{
    uint32_t nodeNum_;
    uint32_t mirrorNum_;

    Topology()
    : nodeNum_(0), mirrorNum_(0)
    {}
};

/**
 * Information of a single node
 */
struct SF1NodeInfo
{
    mirrorid_t mirrorId_;
    nodeid_t nodeId_;
    std::string localHost_;
    uint32_t baPort_;

    SF1NodeInfo()
    : mirrorId_(0)
    , nodeId_(0)
    , baPort_(0)
    {}
};

/// Utility
class NodeUtil
{
public:
    static std::string getSF1RootPath()
    {
        return std::string(ZK_PATH_SF1);
    }

    static std::string getSF1TopologyPath()
    {
        return std::string(ZK_PATH_SF1_TOPOLOGY);
    }

    static std::string getSF1ServicePath()
    {
        return std::string(ZK_PATH_SF1_SERVICE);
    }

    static std::string getMirrorPath(mirrorid_t mirrorId)
    {
        std::stringstream ss;
        ss <<ZK_PATH_SF1_TOPOLOGY<<"/Mirror"<<mirrorId;

        return ss.str();
    }

    static std::string getNodePath(mirrorid_t mirrorId, nodeid_t nodeId)
    {
        std::stringstream ss;
        ss <<ZK_PATH_SF1_TOPOLOGY<<"/Mirror"<<mirrorId<<"/Node"<<nodeId;

        return ss.str();
    }

    static void serialize(std::string& data, std::string& k, std::string& v)
    {
        if (!data.empty())
            data += "$";

        data += k+"#"+v;
    }

    static void deserialize(std::string& data, std::map<std::string, std::string>& kvData)
    {
    }

    static const char delimiter_record = '$';
    static const char delimiter_kv = '#';
};

}

#endif /* NODE_DEF_H_ */
