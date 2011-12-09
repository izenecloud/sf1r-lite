/**
 * @file ZooKeeperManager.h
 * @author Zhongxia Li
 * @date Dec 9, 2011
 * @brief Manage zookeeper clients for distributed coordination tasks.
 */
#ifndef ZOO_KEEPER_MANAGER_H_
#define ZOO_KEEPER_MANAGER_H_

#include <3rdparty/zookeeper/ZooKeeper.hpp>
#include <3rdparty/zookeeper/ZooKeeperEvent.hpp>
#include <util/singleton.h>

#include <boost/shared_ptr.hpp>

using namespace zookeeper;

namespace sf1r
{

typedef boost::shared_ptr<ZooKeeper> ZooKeeperClientPtr;

class ZooKeeperClientFactory
{
public:
    static ZooKeeperClientPtr createZkClient(
            const std::string& hosts,
            const int recvTimeout = 2000,
            bool tryReconnect = false);
};

class ZooKeeperManager
{
public:
    static ZooKeeperManager* get()
    {
        return izenelib::util::Singleton<ZooKeeperManager>::get();
    }

    void start();

    /**
     * Create a zookeeper client
     * @param hosts
     * @param recvTimeout
     * @param eventHandler Set an event handler for created client if not NULL,
     *                     if needed, more handlers can be set to client directly by user.
     * @return
     */
    ZooKeeperClientPtr createClient(
            const std::string& hosts,
            const int recvTimeout,
            ZooKeeperEventHandler* eventHandler = NULL,
            bool tryReconnect = false);
};





}

#endif /* ZOO_KEEPE_RMANAGER_H_ */
