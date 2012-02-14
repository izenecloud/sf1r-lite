#include "FBTRecommender.h"
#include "ItemFilter.h"
#include "../common/RecommendParam.h"
#include "../common/RecommendItem.h"
#include "../storage/OrderManager.h"

#include <glog/logging.h>

#include <list>

namespace sf1r
{

FBTRecommender::FBTRecommender(
    ItemManager& itemManager,
    OrderManager& orderManager
)
    : Recommender(itemManager)
    , orderManager_(orderManager)
{
}

bool FBTRecommender::recommendImpl_(
    RecommendParam& param,
    ItemFilter& filter,
    std::vector<RecommendItem>& recItemVec
)
{
    if (param.inputItemIds.empty())
    {
        LOG(ERROR) << "failed to recommend for empty input items";
        return false;
    }

    std::list<itemid_t> inputItemList(param.inputItemIds.begin(), param.inputItemIds.end());
    std::list<itemid_t> results;

    if (! orderManager_.getFreqItemSets(param.limit, inputItemList, results, &filter))
    {
        LOG(ERROR) << "failed in OrderManager::getFreqItemSets()";
        return false;
    }

    RecommendItem recItem;
    recItem.weight_ = 1;
    for (std::list<itemid_t>::const_iterator it = results.begin();
        it != results.end(); ++it)
    {
        recItem.item_.setId(*it);
        recItemVec.push_back(recItem);
    }

    return true;
}

} // namespace sf1r
