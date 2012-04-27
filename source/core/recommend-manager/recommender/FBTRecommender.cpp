#include "FBTRecommender.h"
#include "../storage/OrderManager.h"

#include <glog/logging.h>

#include <list>

namespace sf1r
{

FBTRecommender::FBTRecommender(OrderManager& orderManager)
    : orderManager_(orderManager)
{
}

bool FBTRecommender::recommendImpl_(
    RecommendParam& param,
    std::vector<RecommendItem>& recItemVec
)
{
    const std::vector<itemid_t>& inputItemIds = param.inputParam.inputItemIds;
    if (inputItemIds.empty())
    {
        LOG(ERROR) << "failed to recommend for empty input items";
        return false;
    }

    std::list<itemid_t> inputItemList(inputItemIds.begin(), inputItemIds.end());
    std::list<itemid_t> results;

    if (! orderManager_.getFreqItemSets(param.inputParam.limit, inputItemList,
                                        results, &param.inputParam.itemFilter))
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
