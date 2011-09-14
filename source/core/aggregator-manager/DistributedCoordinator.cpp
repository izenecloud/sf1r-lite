#include "DistributedCoordinator.h"

using namespace sf1r;

DistributedCoordinator::DistributedCoordinator(const std::string& zkHosts, const int recvTimeout)
{
    zookeeper_.reset(new ZooKeeper(zkHosts, recvTimeout));
}

void DistributedCoordinator::registerNode(const std::string& zkNode, const std::string& data)
{
    if (!zookeeper_->createZNode(zkNode, data))
    {
        newNodeDataList_.push_back(std::make_pair(zkNode, data));
    }
}

void DistributedCoordinator::run()
{
    while (true)
    {
        if (!zookeeper_->isConnected())
            zookeeper_->connect();

        if (zookeeper_->isConnected())
        {
            process();
        }

        sleep(2);
    }
}

void DistributedCoordinator::process()
{
    std::vector<std::pair<std::string, std::string> >::iterator it;
    for (it = newNodeDataList_.begin(); it != newNodeDataList_.end(); )
    {
        if (zookeeper_->createZNode(it->first, it->second))
        {
            it = newNodeDataList_.erase(it);
        }
        else
        {
            it ++;
        }
    }
}
