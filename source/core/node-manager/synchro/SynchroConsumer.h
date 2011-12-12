/**
 * @file SynchroConsumer.h
 * @author Zhongxia Li
 * @date Oct 23, 2011
 * @brief Distributed data synchronizer component
 */

#ifndef SYNCHRO_CONSUMER_H_
#define SYNCHRO_CONSUMER_H_

#include <node-manager/NodeDef.h>
#include <node-manager/ZooKeeperManager.h>

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>


namespace sf1r{

/**
 * Consumer consumes stuff produced by Producer
 */
class SynchroConsumer : public ZooKeeperEventHandler
{
public:
    typedef boost::function< bool( const std::string& dataPath ) > callback_on_produced_t;

    enum ConsumerStatusType
    {
        CONSUMER_STATUS_INIT,
        CONSUMER_STATUS_WATCHING,
        CONSUMER_STATUS_CONSUMING
    };

public:
    SynchroConsumer(boost::shared_ptr<ZooKeeper>& zookeeper, const std::string& syncZkNode);

    ~SynchroConsumer();

    void watchProducer(
            callback_on_produced_t callback_on_produced,
            bool replyProducer = true);

public:
    virtual void process(ZooKeeperEvent& zkEvent);

    virtual void onNodeCreated(const std::string& path);

    virtual void onDataChanged(const std::string& path);

    virtual void onMonitor();

private:
    void doWatchProducer();

    void resetWatch();

private:
    boost::shared_ptr<ZooKeeper> zookeeper_;

    std::string syncZkNode_;

    ConsumerStatusType consumerStatus_;

    boost::mutex mutex_;
    std::string consumerNodePath_;

    callback_on_produced_t callback_on_produced_;
    bool replyProducer_;

    boost::thread thread_;
};

typedef boost::shared_ptr<SynchroConsumer> SynchroConsumerPtr;

}

#endif /* SYNCHRO_CONSUMER_H_ */
