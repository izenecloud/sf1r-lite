#include "MiningTaskService.h"

namespace sf1r
{
MiningTaskService::MiningTaskService()
{
}

MiningTaskService::~MiningTaskService()
{
}

void MiningTaskService::DoMiningCollection()
{
    miningManager_->DoMiningCollection();
}

}

