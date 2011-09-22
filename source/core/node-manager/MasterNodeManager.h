/**
 * @file MasterNodeManager.h
 * @author Zhongxia Li
 * @date Sep 20, 2011S
 * @brief Management of Master node using ZooKeeper.
 */
#ifndef MASTER_NODE_MANAGER_H_
#define MASTER_NODE_MANAGER_H_

#include "NodeDef.h"

#include <3rdparty/zookeeper/ZooKeeper.hpp>
#include <3rdparty/zookeeper/ZooKeeperEvent.hpp>
#include <util/singleton.h>

#include <boost/shared_ptr.hpp>

using namespace zookeeper;

namespace sf1r
{

class MasterNodeManager : public ZooKeeperEventHandler
{
public:
    MasterNodeManager();

    void initZooKeeper(const std::string& zkHosts, const int recvTimeout);

    void initTopology(uint32_t nodeNum, uint32_t mirrorNum);

    bool isReady()
    {
        return isReady_;
    }

    virtual void process(ZooKeeperEvent& zkEvent);

    /**
     * Action on master has been ready
     */
    void onMasterReady();

private:
    void checkWorkers();

    void registerMasterNode();

private:
    bool isReady_;

    Topology topology_;

    boost::shared_ptr<ZooKeeper> zookeeper_;
};

typedef izenelib::util::Singleton<MasterNodeManager> MasterNodeManagerSingleton;

}

#endif /* MASTER_NODE_MANAGER_H_ */
