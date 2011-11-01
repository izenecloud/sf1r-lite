/**
 * @file TypeDef.h
 * @author Zhongxia Li
 * @date Oct 20, 2011
 * @brief 
 */
#ifndef NONE_M_TYPE_DEF_H_
#define NONE_M_TYPE_DEF_H_

#include <stdint.h>
#include <string>
#include <map>

namespace sf1r {

/// types
typedef uint32_t nodeid_t;
typedef uint32_t replicaid_t;

struct Topology
{
    uint32_t nodeNum_;
    uint32_t workerNum_;

    Topology()
    : nodeNum_(0), workerNum_(0)
    {}
};

/// Information of a single SF1 node
struct SF1NodeInfo
{
    replicaid_t replicaId_;
    nodeid_t nodeId_;
    std::string localHost_;
    uint32_t baPort_;

    SF1NodeInfo()
    : replicaId_(0)
    , nodeId_(0)
    , baPort_(0)
    {}
};


}

#endif /* NONE_M_TYPE_DEF_H_ */
