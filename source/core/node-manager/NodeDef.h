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
#include <sstream>

namespace sf1r {

/// Definitions
typedef uint32_t nodeid_t;
typedef uint32_t mirrorid_t;


const static char SF1ROOT[] = "/SF1Root";

/**
The topology of distributed SF1 service:
(1) A Mirror is a sf1 distribution which provides full search data and functionality.

(2) Mirrors are replicated, Nodes(Workers) between mirrors should be one-to-one correspondence,
    i.e., "/Mirror1/Node1" and "/Mirror1/Node1" should carry the same search data. So that,
    if "/Mirror1/Node2" broken down, Masters in Mirror1 will know that it can switch to worker
    on "/Mirror2/Node2."

(3) Each SF1 node can work as Master or Worker (configurable), mirrorId and nodeId are also configured.

(4) Every master in all mirriors provides full search functionality in the same manner, they are exposed
    to higher-level applications as replicated SF1 servers.

/
|--- SF1
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
**/
struct Topology
{
    uint32_t nodeNum_;
    uint32_t mirrorNum_;

    Topology()
    : nodeNum_(0), mirrorNum_(0)
    {}
};


/// Utility
class NodeUtil
{
public:
    static std::string getSF1RootPath()
    {
        return std::string(SF1ROOT);
    }

    static std::string getMirrorPath(mirrorid_t mirrorId)
    {
        std::stringstream ss;
        ss <<SF1ROOT<<"/Mirror"<<mirrorId;

        return ss.str();
    }

    static std::string getNodePath(mirrorid_t mirrorId, nodeid_t nodeId)
    {
        std::stringstream ss;
        ss <<SF1ROOT<<"/Mirror"<<mirrorId<<"/Node"<<nodeId;

        return ss.str();
    }
};

}

#endif /* NODE_DEF_H_ */
