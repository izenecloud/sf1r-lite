#include "QueryIntentController.h"
#include "CollectionHandler.h"

#include <mining-manager/query-intent/QueryIntentManager.h>
#include <mining-manager/MiningManager.h>
#include <bundles/mining/MiningSearchService.h>

namespace sf1r
{

QueryIntentController::QueryIntentController()
    : Sf1Controller(false)
    , miningSearchService_(NULL)
{
}

bool QueryIntentController::checkCollectionService(std::string& error)
{
    miningSearchService_ = collectionHandler_->miningSearchService_;

    if (miningSearchService_)
        return true;

    error = "Request failed, no mining search service found.";
    return false;
}

void QueryIntentController::reload()
{
    if (!miningSearchService_ || !(miningSearchService_->GetMiningManager()))
        return;
    QueryIntentManager* queryIntent = miningSearchService_->GetMiningManager()->getQueryIntentManager();
    if (queryIntent)
        queryIntent->reload();
    return;
}

}
