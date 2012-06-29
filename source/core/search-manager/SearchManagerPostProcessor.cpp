#include "SearchManagerPostProcessor.h"
//#include <ranking-manager/RankingManager.h>
#include <mining-manager/product-ranker/ProductRankerFactory.h>
#include <mining-manager/product-ranker/ProductRanker.h>
#include <common/SFLogger.h>

#include <util/get.h>
#include <util/ClockTimer.h>
#include <fstream>
#include <algorithm>
#include <boost/scoped_ptr.hpp>

const std::string RANK_PROPERTY("_rank");

namespace sf1r {

bool isProductRanking(const KeywordSearchActionItem& actionItem)
{
    if (actionItem.searchingMode_.mode_ == SearchingMode::KNN)
        return false;

    const KeywordSearchActionItem::SortPriorityList& sortPropertyList = actionItem.sortPriorityList_;
    if (sortPropertyList.empty())
        return true;

    for (KeywordSearchActionItem::SortPriorityList::const_iterator it = sortPropertyList.begin();
        it != sortPropertyList.end(); ++it)
    {
        std::string fieldNameL = it->first;
        boost::to_lower(fieldNameL);
        if (fieldNameL == RANK_PROPERTY)
            return true;
    }

    return false;
}


SearchManagerPostProcessor::SearchManagerPostProcessor()
{
}

SearchManagerPostProcessor::~SearchManagerPostProcessor()
{
}

bool SearchManagerPostProcessor::rerank(const KeywordSearchActionItem& actionItem, KeywordSearchResult& resultItem)
{
    if (productRankerFactory_ &&
        resultItem.topKCustomRankScoreList_.empty() &&
        isProductRanking(actionItem))
    {
        izenelib::util::ClockTimer timer;

        ProductRankingParam rankingParam(actionItem.env_.queryString_,
            resultItem.topKDocs_, resultItem.topKRankScoreList_, actionItem.groupParam_);

        boost::scoped_ptr<ProductRanker> productRanker(
            productRankerFactory_->createProductRanker(rankingParam));

        productRanker->rank();

        LOG(INFO) << "topK doc num: " << resultItem.topKDocs_.size()
                  << ", product ranking costs: " << timer.elapsed() << " seconds";

        return true;
    }
    return false;
}

}
