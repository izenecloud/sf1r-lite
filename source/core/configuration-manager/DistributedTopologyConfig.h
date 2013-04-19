/**
 * @file DistributedTopologyConfig.h
 * @author Zhongxia Li
 * @date Sep 21, 2011
 * @brief 
 */
#ifndef DISTRIBUTED_CONFIG_H_
#define DISTRIBUTED_CONFIG_H_

#include <node-manager/Sf1rTopology.h>


#include <sstream>

namespace sf1r
{

class DistributedCommonConfig
{
public:
    std::string toString()
    {
        std::stringstream ss;
        ss << "[DistributedCommonConfig]" << std::endl
           << "cluster id: " << clusterId_ << std::endl
           << "username: " << userName_ << std::endl
           << "localhost: " << localHost_ << std::endl
           << "BA Port: " << baPort_ << std::endl
           << "worker server port: " << workerPort_ << std::endl
           << "master server port: " << masterPort_  << std::endl
           << "data receiver port: " << dataRecvPort_ << std::endl
           << "file sync rpc port: " << filesync_rpcport_ << std::endl;
        return ss.str();
    }

public:
    std::string clusterId_;
    std::string userName_;
    std::string localHost_;
    unsigned int baPort_;
    unsigned int workerPort_;
    unsigned int masterPort_;
    unsigned int dataRecvPort_;
    unsigned int filesync_rpcport_;
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
        ss << "==== [DistributedTopology] ====" << std::endl;
        ss << (enabled_ ? "enabled":"disabled") << std::endl;
        ss << sf1rTopology_.toString();
        ss << "==============================="<<std::endl;
        return ss.str();
    }

public:
    bool enabled_;
    Sf1rTopology sf1rTopology_;
};


}

#endif /* DISTRIBUTED_CONFIG_H_ */
