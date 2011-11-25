#ifndef ZK_MONITOR_H_
#define ZK_MONITOR_H_

#include <vector>
#include <util/singleton.h>

#include <boost/thread.hpp>
#include <boost/function.hpp>

typedef boost::function0<void> monitor_handler_t;

namespace sf1r{

/**
 * ZooKeeper Monitor
 * In case, such as network interrupted, clients will loss connection to zookeeper service,
 * automatically reconnecting and recover(reset watcher etc.) needed after network resumed.
 */
class ZkMonitor
{
public:
    ZkMonitor();

    ~ZkMonitor();

    static ZkMonitor* get()
    {
        return ::izenelib::util::Singleton<ZkMonitor>::get();
    }

    void addMonitorHandler(monitor_handler_t handler);

    void start();

    void stop();

private:
    void loopMonitor_();

    void doMonitor_();

private:
    boost::thread thread_;

    long monitorInterval_;
    std::vector<monitor_handler_t> monitorHandlers_;
};

}

#endif /* NODE_MONITOR_THREAD_H_ */
