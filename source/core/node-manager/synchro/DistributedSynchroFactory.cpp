#include "DistributedSynchroFactory.h"

using namespace sf1r;

std::string DistributedSynchroFactory::synchroType2ZNode_[SYNCHRO_TYPE_NUM] =
{
    "/ProductManager" // SYNCHRO_TYPE_PRODUCT_MANAGER  = 0
};

SynchroProducerPtr DistributedSynchroFactory::syncProducer_[SYNCHRO_TYPE_NUM];
SynchroConsumerPtr DistributedSynchroFactory::syncConsumer_[SYNCHRO_TYPE_NUM];
