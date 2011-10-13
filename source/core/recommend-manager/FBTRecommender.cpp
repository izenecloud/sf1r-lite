#include "FBTRecommender.h"
#include "OrderManager.h"
#include "ItemFilter.h"

#include <glog/logging.h>

#include <list>

namespace sf1r
{

FBTRecommender::FBTRecommender(OrderManager* orderManager)
    : orderManager_(orderManager)
{
}

bool FBTRecommender::recommend(
    int maxRecNum,
    const std::vector<itemid_t>& inputItemVec,
    ItemFilter& filter,
    std::vector<RecommendItem>& recItemVec
)
{
    if (inputItemVec.empty())
    {
        LOG(ERROR) << "failed to recommend for empty input items";
        return false;
    }

    std::list<itemid_t> inputItemList(inputItemVec.begin(), inputItemVec.end());
    std::list<itemid_t> results;
    if (orderManager_->getFreqItemSets(maxRecNum, inputItemList, results, &filter) == false)
    {
        LOG(ERROR) << "failed in OrderManager::getFreqItemSets()";
        return false;
    }

    RecommendItem recItem;
    recItem.weight_ = 1;
    for (std::list<itemid_t>::const_iterator it = results.begin();
        it != results.end(); ++it)
    {
        recItem.itemId_ = *it;
        recItemVec.push_back(recItem);
    }

    return true;
}

} // namespace sf1r
