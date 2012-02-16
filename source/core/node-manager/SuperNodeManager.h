/**
 * @file SuperNodeManager.h
 * @author Zhongxia Li
 * @date Dec 13, 2011
 * @brief Management for node common settings
 */
#ifndef SUPER_NODE_MANAGER_H_
#define SUPER_NODE_MANAGER_H_

#include <configuration-manager/DistributedTopologyConfig.h>
#include <configuration-manager/DistributedUtilConfig.h>

#include <util/singleton.h>

namespace sf1r
{

class SuperNodeManager
{
public:
    static SuperNodeManager* get()
    {
        return izenelib::util::Singleton<SuperNodeManager>::get();
    }

    void init(const DistributedCommonConfig& distributedCommon)
    {
        distributedCommon_ = distributedCommon;

        //xxx check clustid, host
    }

    const DistributedCommonConfig& getCommonConfig()
    {
        return distributedCommon_;
    }

    const std::string& getClusterId()
    {
        return distributedCommon_.clusterId_;
    }

    const std::string& getLocalHostIP()
    {
        return distributedCommon_.localHost_;
    }

    unsigned int getBaPort() const
    {
        return distributedCommon_.baPort_;
    }

    unsigned int getWorkerPort() const
    {
        return distributedCommon_.workerPort_;
    }

    unsigned int getMasterPort() const
    {
        return distributedCommon_.masterPort_;
    }

    unsigned int getDataReceiverPort() const
    {
        return distributedCommon_.dataRecvPort_;
    }

private:
    DistributedCommonConfig distributedCommon_;
};

}

#endif /* SUPER_NODE_MANAGER_H_ */
