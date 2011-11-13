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
#include <sstream>

namespace sf1r {

/// types
typedef uint32_t nodeid_t;
typedef uint32_t replicaid_t;
typedef uint32_t shardid_t;


struct Topology
{
    std::string clusterId_;
    uint32_t nodeNum_;
    uint32_t shardNum_;

    Topology()
    : nodeNum_(0), shardNum_(0)
    {}
};

struct SF1NodeInfo
{
    replicaid_t replicaId_;
    nodeid_t nodeId_;

    std::string host_;
    uint32_t baPort_;

    SF1NodeInfo()
    : replicaId_(0)
    , nodeId_(0)
    , baPort_(0)
    {}

    std::string toString()
    {
        std::stringstream ss;
        ss <<"[Replica "<<replicaId_<<", Node "<<nodeId_<<"] "
           <<host_<<" :baPort"<<baPort_;
        return ss.str();
    }
};

struct MasterNode : public SF1NodeInfo
{
    uint32_t masterPort_;

    std::string toString()
    {
        std::stringstream ss;
        ss <<SF1NodeInfo::toString()
           <<" :master"<<masterPort_;
        return ss.str();
    }
};

struct WorkerNode : public SF1NodeInfo
{
    shardid_t shardId_;
    uint32_t workerPort_;

    WorkerNode()
    : shardId_(0)
    , workerPort_(0)
    {}

    std::string toString()
    {
        std::stringstream ss;
        ss <<SF1NodeInfo::toString()
           <<" :worker"<<workerPort_<<", shard"<<shardId_;
        return ss.str();
    }
};

}

#endif /* NONE_M_TYPE_DEF_H_ */
