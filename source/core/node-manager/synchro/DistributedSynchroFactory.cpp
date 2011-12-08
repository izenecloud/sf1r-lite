#include "DistributedSynchroFactory.h"

using namespace sf1r;

std::string DistributedSynchroFactory::synchroType2ZNode_[SYNCHRO_TYPE_NUM] =
{
    "/ProductManager" // SYNCHRO_TYPE_PRODUCT_MANAGER  = 0
};

std::map<std::string, SynchroProducerPtr> DistributedSynchroFactory::syncProducer_;
std::map<std::string, SynchroConsumerPtr> DistributedSynchroFactory::syncConsumer_;
