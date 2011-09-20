/**
 * @file WorkerDetector.h
 * @author Zhongxia Li
 * @date Sep 14, 2011
 * @brief 
 */
#ifndef WORKER_DECECTOR_H_
#define WORKER_DECECTOR_H_

#include <3rdparty/zookeeper/ZooKeeper.hpp>
#include <3rdparty/zookeeper/ZooKeeperWatcher.hpp>
#include <3rdparty/zookeeper/ZooKeeperEvent.hpp>


using namespace zookeeper;

namespace sf1r{

class WorkerDetector : public ZooKeeperEventHandler
{
public:
    virtual void process(ZooKeeperEvent& zkEvent);

    virtual void onNodeCreated(const std::string& path);
};


}

#endif /* WORKER_DECECTOR_H_ */
