#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>

#include <iostream>
#include "string.h"

#include <node-manager/synchro/DistributedSynchroFactory.h>

using namespace sf1r;

void callback_on_consumed(bool isSuccess)
{
    cout <<"--> callback on consume finished: "<<isSuccess<<endl;
}

bool callback_on_produced(const std::string& datapath)
{
    cout<<"--> callback on produced: "<<datapath<<endl;

    cout<<"Consuming ..." <<endl;
    sleep(2);

    return true;
}

void thread_consumer_run()
{
    SynchroConsumerPtr scp =
            DistributedSynchroFactory::makeConsumer(DistributedSynchroFactory::SYNCHRO_TYPE_PRODUCT_MANAGER);

    scp->watchProducer(callback_on_produced, true);
}

int main(int argc, char** argv)
{
    if (argc > 1)
    {
        for (int i = 1; i < argc; i++)
        {
            // disable auto test
            if (strcasecmp(argv[i], "--build_info") == 0)
                exit(0);
        }
    }

    // set default config
    DistributedTopologyConfig dsTopologyConfig;
    dsTopologyConfig.clusterId_ = "zhongxia";
    dsTopologyConfig.nodeNum_ = 2;
    dsTopologyConfig.curSF1Node_.host_ = "172.16.0.36";
    dsTopologyConfig.curSF1Node_.nodeId_ = 1;
    dsTopologyConfig.curSF1Node_.replicaId_ = 1;
    DistributedUtilConfig dsUtilConfig;
    dsUtilConfig.zkConfig_.zkHosts_ = "172.16.0.161:2181,172.16.0.162:2181,172.16.0.163:2181";
    dsUtilConfig.zkConfig_.zkRecvTimeout_ = 2000;

    NodeManagerSingleton::get()->initWithConfig(dsTopologyConfig, dsUtilConfig);

    // Consumer
    boost::thread consumer_thread(thread_consumer_run);

    // Producer
    {
        SynchroProducerPtr spd =
            DistributedSynchroFactory::makeProducer(DistributedSynchroFactory::SYNCHRO_TYPE_PRODUCT_MANAGER);
        spd->produce("/data/scd", callback_on_consumed);
        bool ret;
        spd->waitConsumers(ret);
        cout << "Producer: wait consumers ended " <<ret<<endl;

        //spd->produce("/data/scd2", callback_on_consumed);
        //sleep(3);
    }

    consumer_thread.join();

    exit(0);
}
