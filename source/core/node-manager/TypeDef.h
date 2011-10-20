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
typedef uint32_t mirrorid_t;

struct Topology
{
    uint32_t nodeNum_;
    uint32_t mirrorNum_;

    Topology()
    : nodeNum_(0), mirrorNum_(0)
    {}
};

/// Information of a single SF1 node
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


}

#endif /* NONE_M_TYPE_DEF_H_ */
