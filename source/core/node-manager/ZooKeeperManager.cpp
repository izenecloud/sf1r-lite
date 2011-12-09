#include "ZooKeeperManager.h"


namespace sf1r
{

ZooKeeperClientPtr
ZooKeeperClientFactory::createZkClient(
        const std::string& hosts,
        const int recvTimeout,
        bool tryReconnect)
{
    ZooKeeperClientPtr ret(new ZooKeeper(hosts, recvTimeout, tryReconnect));
    return ret;
}


void ZooKeeperManager::start()
{

}

ZooKeeperClientPtr ZooKeeperManager::createClient(
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
    }

    return ret;
}

}
