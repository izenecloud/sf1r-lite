/**
 * @file DistributedCoordinator.h
 * @author Zhongxia Li
 * @date Sep 14, 2011
 * @brief 
 */
#ifndef DISTRIBUTED_COORDINATOR_H_
#define DISTRIBUTED_COORDINATOR_H_

#include <3rdparty/zookeeper/ZooKeeper.hpp>

#include <boost/shared_ptr.hpp>

using namespace zookeeper;

namespace sf1r{


class DistributedCoordinator
{
public:
    /**
     * @param zkHosts ZooKeeper servers' addresses
     * @param recvTimeout timeout for connecting ZooKeeper
     */
    DistributedCoordinator(const std::string& zkHosts, const int recvTimeout);

    void registerNode(const std::string& zkNode, const std::string& data="");

    void run();

    void process();

private:
    boost::shared_ptr<ZooKeeper> zookeeper_;

    std::vector<std::pair<std::string, std::string> > newNodeDataList_;
};

}

#endif /* DISTRIBUTED_COORDINATOR_H_ */
