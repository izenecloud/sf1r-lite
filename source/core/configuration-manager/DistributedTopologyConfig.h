/**
 * @file DistributedTopologyConfig.h
 * @author Zhongxia Li
 * @date Sep 21, 2011
 * @brief 
 */
#ifndef DISTRIBUTED_CONFIG_H_
#define DISTRIBUTED_CONFIG_H_

#include "MasterAgentConfig.h"
#include "WorkerAgentConfig.h"


#include <sstream>

namespace sf1r
{

struct SF1Node
{
    SF1Node()
        : nodeId_(0)
        , replicaId_(0)
    {}

    unsigned int nodeId_;
    unsigned int replicaId_;

    MasterAgentConfig masterAgent_;
    WorkerAgentConfig workerAgent_;

    std::string toString()
    {
        std::stringstream ss;
        ss << "[Current SF1 Node] nodeId: "<<nodeId_<<", replicaId: "<<replicaId_<<std::endl;
        ss << masterAgent_.toString();
        ss << workerAgent_.toString();

        return ss.str();
    }
};

class DistributedTopologyConfig
{
public:
    DistributedTopologyConfig()
    : enabled_(false), nodeNum_(0)
    {
    }

    std::string toString()
    {
        std::stringstream ss;
        ss << "==== [DistributedTopology] ===="<<std::endl;
        ss << (enabled_ ? "enabled":"disabled")
           <<" : nodeNum:" << nodeNum_ << std::endl;

        ss << curSF1Node_.toString() << std::endl;
        ss << "==============================="<<std::endl;
        return ss.str();
    }

public:
    // golable topology configuration
    bool enabled_;
    unsigned int nodeNum_;

    // current SF1 node configuration
    SF1Node curSF1Node_;
};

class DistributedCommonConfig
{
public:

    std::string toString()
    {
        std::stringstream ss;
        ss <<"[DistributedCommonConfig] Cluster Id: "<<clusterId_<<", Username: "<<userName_<<", Local Host: "<<localHost_
           <<", BA Port: "<<baPort_<<", Worker Port:"<<workerPort_
           <<", Master Port:"<<masterPort_<<", Data Receiver Port:"<<dataRecvPort_;
        return ss.str();
    }

    std::string clusterId_;
    std::string userName_;
    std::string localHost_;
    unsigned int baPort_;
    unsigned int workerPort_;
    unsigned int masterPort_;
    unsigned int dataRecvPort_;
};

}

#endif /* DISTRIBUTED_CONFIG_H_ */
