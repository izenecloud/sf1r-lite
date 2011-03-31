#include "QueryLogTaskService.h"

namespace sf1r
{
QueryLogTaskService::QueryLogTaskService()
{
}

QueryLogTaskService::~QueryLogTaskService()
{
}

void QueryLogTaskService::DoMiningCollection()
{
    miningManager_->DoMiningCollection();
}

}

