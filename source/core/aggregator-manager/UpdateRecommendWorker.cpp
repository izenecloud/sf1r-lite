#include "UpdateRecommendWorker.h"

#include <glog/logging.h>
#include <boost/bind.hpp>

namespace sf1r
{

UpdateRecommendWorker::UpdateRecommendWorker(
    ItemCFManager& itemCFManager,
    CoVisitManager& coVisitManager
)
    : itemCFManager_(itemCFManager)
    , coVisitManager_(coVisitManager)
{
}

void UpdateRecommendWorker::updatePurchaseMatrix(
    const std::list<itemid_t>& oldItems,
    const std::list<itemid_t>& newItems,
    bool& result
)
{
    itemCFManager_.updateMatrix(oldItems, newItems);
    result = true;
}

void UpdateRecommendWorker::updatePurchaseCoVisitMatrix(
    const std::list<itemid_t>& oldItems,
    const std::list<itemid_t>& newItems,
    bool& result
)
{
    itemCFManager_.updateVisitMatrix(oldItems, newItems);
    result = true;
}

void UpdateRecommendWorker::buildPurchaseSimMatrix(bool& result)
{
    jobScheduler_.addTask(boost::bind(&ItemCFManager::buildSimMatrix, &itemCFManager_));
    result = true;
}

void UpdateRecommendWorker::updateVisitMatrix(
    const std::list<itemid_t>& oldItems,
    const std::list<itemid_t>& newItems,
    bool& result
)
{
    coVisitManager_.visit(oldItems, newItems);
    result = true;
}

void UpdateRecommendWorker::flushRecommendMatrix(bool& result)
{
    jobScheduler_.addTask(boost::bind(&UpdateRecommendWorker::flushImpl_, this));
    result = true;
}

void UpdateRecommendWorker::flushImpl_()
{
    coVisitManager_.flush();
    itemCFManager_.flush();

    LOG(INFO) << "flushed [Visit] " << coVisitManager_.matrix();
    LOG(INFO) << "flushed [Purchase] " << itemCFManager_;
}

} // namespace sf1r
