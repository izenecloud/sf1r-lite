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
    unsigned int nodeId_;
    unsigned int mirrorId_;
    std::string host_;

    MasterAgentConfig masterAgent_;
    WorkerAgentConfig workerAgent_;

    std::string toString()
    {
        std::stringstream ss;
        ss << "--[Current SF1 Node] nodeId: "<<nodeId_<<" mirrorId: "<<mirrorId_<<" host: "<<host_<<endl;

        ss << masterAgent_.toString();
        ss << workerAgent_.toString();

        return ss.str();
    }
};


class DistributedTopologyConfig
{
public:
    DistributedTopologyConfig()
    : enabled_(false)
    {
    }

    std::string toString()
    {
        std::stringstream ss;
        ss << "==== [DistributedTopology] ===="<<endl;
        ss << "enabled ? "<<enabled_
           <<" nodeNum: "<<nodeNum_<<" mirrorNum: "<<mirrorNum_<<endl;

        ss << curSF1Node_.toString();
        ss << "==============================="<<endl;
        return ss.str();
    }

public:
    // golable topology configuration
    bool enabled_;
    unsigned int nodeNum_;
    unsigned int mirrorNum_;

    // current SF1 node configuration
    SF1Node curSF1Node_;
};

}

#endif /* DISTRIBUTED_CONFIG_H_ */
