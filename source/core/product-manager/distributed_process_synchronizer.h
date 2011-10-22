/**
 * @file distributed_process_synchronizer.h
 * @author Zhongxia Li
 * @date Oct 18, 2011
 * @brief 
 */
#ifndef DISTRIBUTED_PROCESS_SYNCHRONIZER_H_
#define DISTRIBUTED_PROCESS_SYNCHRONIZER_H_

#include <3rdparty/zookeeper/ZooKeeper.hpp>
#include <3rdparty/zookeeper/ZooKeeperEvent.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

using namespace zookeeper;

namespace sf1r
{

class DistributedProcessSynchronizer : public ZooKeeperEventHandler
{
public:
    typedef boost::function< void( const std::string& ) > callback_on_generated_t;
    typedef boost::function< void( bool ) > callback_on_processed_t;

    DistributedProcessSynchronizer();

public:
    bool generated(std::string& path);

    bool processed(bool success);

    void watchGenerate(callback_on_generated_t callback_on_generated);

    void watchProcess(callback_on_processed_t callback_on_processed);

    void joinProcess(bool& result);

public:
    virtual void process(ZooKeeperEvent& zkEvent);

    virtual void onNodeCreated(const std::string& path);

    void processOnGenerated();

    void processOnProcessed();

private:
    void initOnConnected();

    void do_generated(std::string& path);

    void do_processed(bool success);

private:
    callback_on_generated_t callback_on_generated_;
    callback_on_processed_t callback_on_processed_;

    boost::shared_ptr<ZooKeeper> zookeeper_;
    std::string prodNodePath_;
    std::string generateNodePath_;
    std::string processNodePath_;
    bool isZkConnected_;

    bool generated_;
    bool generated_znode_;
    std::string generatedDataPath_;
    bool watched_process_;
    bool process_result_;

    bool processed_;
    bool processed_znode_;
    bool processedSuccess_;
};

}

#endif /* DISTRIBUTED_PROCESS_SYNCHRONIZER_H_ */
