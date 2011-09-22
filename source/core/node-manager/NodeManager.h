/**
 * @file NodeManager.h
 * @author Zhongxia Li
 * @date Sep 20, 2011
 * @brief Management of SF1 node using ZooKeeper.
 */
#ifndef NODE_MANAGER_H_
#define NODE_MANAGER_H_

#include "NodeDef.h"

#include <3rdparty/zookeeper/ZooKeeper.hpp>
#include <3rdparty/zookeeper/ZooKeeperEvent.hpp>
#include <util/singleton.h>

#include <boost/shared_ptr.hpp>

using namespace zookeeper;

namespace sf1r
{

class NodeManager : public ZooKeeperEventHandler
{
public:
    NodeManager();

    void initZooKeeper(const std::string& zkHosts, const int recvTimeout);

    /**
     * Register SF1 node when start up.
     * @param nodeId
     * @param mirrorId
     * @param data
     */
    void registerNode(nodeid_t nodeId, mirrorid_t mirrorId, const std::string& data="");

    /**
     * Register Master on current SF1 node.
     */
    void registerNodeMaster(nodeid_t nodeId, mirrorid_t mirrorId, const std::string& data="");

    virtual void process(ZooKeeperEvent& zkEvent);

private:
    void doProcess();

private:
    boost::shared_ptr<ZooKeeper> zookeeper_;

    std::vector<std::pair<std::string, std::string> > registerTaskList_;

};


typedef izenelib::util::Singleton<NodeManager> NodeManagerSingleton;

}

#endif /* NODE_MANAGER_H_ */
