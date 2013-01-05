/**
 * @file SynchroConsumer.h
 * @author Zhongxia Li
 * @date Oct 23, 2011
 * @brief Distributed synchronizer works as Producer-Consumer, Consumer consumes data produced by Producer.
 */

#ifndef SYNCHRO_CONSUMER_H_
#define SYNCHRO_CONSUMER_H_

#include "SynchroData.h"

#include <node-manager/ZooKeeperManager.h>

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread.hpp>


namespace sf1r{

/**
 * ZooKeeper namespace for Synchro Producer and Consumer(s).
 *
 * SynchroNode
 *  |--- Producer
 *  |--- Consumers
 *       |--- Consumer00000000
 *       |--- Consumer00000001
 *       |--- ...
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
            const std::string& collectionName);

public:
    virtual void process(ZooKeeperEvent& zkEvent);

    virtual void onNodeCreated(const std::string& path);

    virtual void onDataChanged(const std::string& path);

    virtual void onNodeDeleted(const std::string& path);

    virtual void onMonitor();

private:
    void doWatchProducer();

    bool synchronize();

    bool consume(SynchroData& producerMsg);

    void resetWatch();

private:
    boost::shared_ptr<ZooKeeper> zookeeper_;

    std::string syncZkNode_;
    std::string producerZkNode_;

    ConsumerStatusType consumerStatus_;

    boost::mutex mutex_;
    std::string consumerNodePath_;

    callback_on_produced_t callback_on_produced_;

    std::string collectionName_;
};

typedef boost::shared_ptr<SynchroConsumer> SynchroConsumerPtr;

}

#endif /* SYNCHRO_CONSUMER_H_ */
