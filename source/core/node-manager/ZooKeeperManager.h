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

#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/function.hpp>

using namespace izenelib::zookeeper;

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
    ZooKeeperManager();

    ~ZooKeeperManager();

    static ZooKeeperManager* get()
    {
        return izenelib::util::Singleton<ZooKeeperManager>::get();
    }

    void start();

    void stop();

    /**
     * Create a zookeeper client, all ZooKeeper Client should be created using this interface.
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

    /**
     * Register monitor event handler
     * @param evtHandler
     */
    void registerMonitorEventHandler(ZooKeeperEventHandler* evtHandler)
    {
        clientKeeperList_.push_back(evtHandler);
    }

private:
    /**
     * ZooKeeper service is stable, but in cases, such as network was interrupted,
     * clients will loss connection to service and Watchers will lose efficacy.
     * Auto monitoring is used to check connection status and recover.
     */
    void monitorLoop();

    void postMonitorEvent();

private:
    std::vector<ZooKeeperEventHandler*> clientKeeperList_;

    long monitorInterval_;
    boost::thread monitorThread_;
};





}

#endif /* ZOO_KEEPE_RMANAGER_H_ */
