/**
 * @file SynchroProducer.h
 * @author Zhongxia Li
 * @date Oct 23, 2011
 * @brief Distributed data synchronizer component
 */

#ifndef SYNCHROPRODUCER_H_
#define SYNCHROPRODUCER_H_

#include <3rdparty/zookeeper/ZooKeeper.hpp>
#include <3rdparty/zookeeper/ZooKeeperEvent.hpp>

#include <node-manager/NodeDef.h>

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>

#include <map>

using namespace zookeeper;

namespace sf1r{

class SynchroProducer : public ZooKeeperEventHandler
{
public:
    typedef boost::function< void( bool ) > callback_on_consumed_t;

public:
    SynchroProducer(
            const std::string& zkHosts,
            int zkTimeout,
            const std::string zkSyncNodePath,
            replicaid_t replicaId,
            nodeid_t nodeId
            );

    ~SynchroProducer();

    /**
     * Produce data
     * @param dataPath
     * @param callback_on_consumed callback on data consumed by consumers,
     *        Note: if callback is set,
     * @return true if success, else false;
     */
    bool produce(const std::string& dataPath, callback_on_consumed_t callback_on_consumed = NULL);

    /**
     *
     * @param isSuccess
     * @param findConsumerTimeout  timeout (seconds) for saw any consumer.
     * @return
     */
    bool waitConsumers(bool& isConsumed, int findConsumerTimeout = 120);

public:
    virtual void process(ZooKeeperEvent& zkEvent);

    virtual void onNodeDeleted(const std::string& path);
    virtual void onDataChanged(const std::string& path);
    virtual void onChildrenChanged(const std::string& path);

private:
    void ensureParentNodes();

    void init();

    bool doProcude(const std::string& dataPath);

    void watchConsumers();

    void checkConsumers();

    void endSynchroning();


private:
    boost::shared_ptr<ZooKeeper> zookeeper_;

    std::string syncNodePath_;
    std::string prodNodePath_;

    bool isSynchronizing_; // perform one synchronization work at a time, xxx

    bool watchedConsumer_;
    typedef std::map<std::string, std::pair<bool,bool> > consumermap_t;
    consumermap_t consumersMap_; // consumer-path, <finished?,result?>
    size_t consumedCount_;
    callback_on_consumed_t callback_on_consumed_;
    bool result_on_consumed_;

    boost::mutex consumers_mutex_;
};

typedef boost::shared_ptr<SynchroProducer> SynchroProducerPtr;

}

#endif /* SYNCHROPRODUCER_H_ */
