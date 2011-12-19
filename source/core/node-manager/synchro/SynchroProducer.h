/**
 * @file SynchroProducer.h
 * @author Zhongxia Li
 * @date Oct 23, 2011
 * @brief Distributed synchronizer works as Producer-Consumer, Producer produces data to be synchronized.
 */

#ifndef SYNCHROPRODUCER_H_
#define SYNCHROPRODUCER_H_

#include "SynchroData.h"

#include <node-manager/ZooKeeperManager.h>

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>

#include <map>


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
class SynchroProducer : public ZooKeeperEventHandler
{
public:
    typedef boost::function< void( bool ) > callback_on_consumed_t;

    enum DataTransferPolicy
    {
        DATA_TRANSFER_POLICY_DFS,     // transfer data via DFS (Distributed File System)
        DATA_TRANSFER_POLICY_SOCKET   // transfer data via socket
    };

public:
    SynchroProducer(
            boost::shared_ptr<ZooKeeper>& zookeeper,
            const std::string& syncZkNode,
            DataTransferPolicy transferPolicy = DATA_TRANSFER_POLICY_SOCKET);

    ~SynchroProducer();

    void setDataTransferPolicy(DataTransferPolicy transferPolicy)
    {
        transferPolicy_ = transferPolicy;
    }

    /**
     * Produce synchro data (asynchronously)
     * @brief After produced synchro data, there is no assurance that synchronizing will be finished
     *        (consumed by consumer) or succeed. Call wait() to wait for synchronizing to finish and get status.
     *
     * @param syncData  synchro data
     * @param callback_on_consumed  callback on data consumed by consumers
     * @return true if successfully published synchro data, else false;
     */
    bool produce(SynchroData& syncData, callback_on_consumed_t callback_on_consumed = NULL);

    /**
     * Wait for synchronizing to finish
     * @param timeout  timeout for waiting until watched at least one consumer,
     *                 once watched any consumer there will be no timeout.
     * @return true on successfully synchronized, false on failed.
     */
    bool wait(int timeout = 150);

public:
    virtual void process(ZooKeeperEvent& zkEvent);

    virtual void onNodeDeleted(const std::string& path);
    virtual void onDataChanged(const std::string& path);
    virtual void onChildrenChanged(const std::string& path);

private:
    void init();

    bool doProduce(SynchroData& syncData);

    void watchConsumers();
    bool transferData(const std::string& consumerZNodePath);

    void checkConsumers();

    void endSynchroning(const std::string& info="success");


private:
    struct ConsumerStatus
    {
        std::string znodePath_;

    };

    DataTransferPolicy transferPolicy_;

    boost::shared_ptr<ZooKeeper> zookeeper_;

    std::string syncZkNode_;
    std::string producerZkNode_;

    bool isSynchronizing_;
    SynchroData syncData_;

    bool watchedConsumer_;
    typedef std::map<std::string, std::pair<bool,bool> > consumermap_t;
    consumermap_t consumersMap_; // consumer-path, <finished?,result?>
    size_t consumedCount_;
    callback_on_consumed_t callback_on_consumed_;
    bool result_on_consumed_;

    boost::mutex produce_mutex_;
    boost::mutex consumers_mutex_;
};

typedef boost::shared_ptr<SynchroProducer> SynchroProducerPtr;

}

#endif /* SYNCHROPRODUCER_H_ */
