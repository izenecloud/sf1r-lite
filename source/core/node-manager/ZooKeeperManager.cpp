#include "ZooKeeperManager.h"


namespace sf1r
{

/// ZooKeeperClientFactory
ZooKeeperClientPtr
ZooKeeperClientFactory::createZkClient(
        const std::string& hosts,
        const int recvTimeout,
        bool tryReconnect)
{
    ZooKeeperClientPtr ret(new ZooKeeper(hosts, recvTimeout, tryReconnect));
    return ret;
}

/// ZooKeeperManager
const static long MONOTOR_INTERVAL_SECONDS = 120;

ZooKeeperManager::ZooKeeperManager()
: monitorInterval_(MONOTOR_INTERVAL_SECONDS)
{
    // start monitor thread
    monitorThread_ = boost::thread(&ZooKeeperManager::monitorLoop, this);
}

ZooKeeperManager::~ZooKeeperManager()
{
    stop();
}

void ZooKeeperManager::start()
{

}

void ZooKeeperManager::stop()
{
    monitorThread_.interrupt();
    monitorThread_.join();
}

ZooKeeperClientPtr
ZooKeeperManager::createClient(
        const std::string& hosts,
        const int recvTimeout,
        ZooKeeperEventHandler* eventHandler,
        bool tryReconnect)
{
    ZooKeeperClientPtr ret;
    ret = ZooKeeperClientFactory::createZkClient(hosts, recvTimeout, tryReconnect);

    if (NULL != eventHandler)
    {
        ret->registerEventHandler(eventHandler);
        registerMonitorEventHandler(eventHandler);
    }

    return ret;
}

// private
void ZooKeeperManager::monitorLoop()
{
    std::cout << "[ZooKeeperManager] monitorLoop " <<std::endl;

    while (true)
    {
        try
        {
            boost::this_thread::sleep(boost::posix_time::seconds(monitorInterval_));

            postMonitorEvent();
        }
        catch (std::exception& e)
        {
            //break;
        }
    }
}

void ZooKeeperManager::postMonitorEvent()
{
    std::vector<ZooKeeperEventHandler*>::iterator it;
    for (it = clientKeeperList_.begin(); it != clientKeeperList_.end(); it++)
    {
        (*it)->onMonitor();
    }
}

}
