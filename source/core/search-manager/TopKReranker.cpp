#include "TopKReranker.h"
#include "SearchManagerPreProcessor.h"
#include <query-manager/ActionItem.h>
#include <common/ResultType.h>
#include <mining-manager/product-ranker/ProductRankerFactory.h>
#include <mining-manager/product-ranker/ProductRankParam.h>
#include <mining-manager/product-ranker/ProductRanker.h>
#include <util/ClockTimer.h>

#include <glog/logging.h>
#include <boost/scoped_ptr.hpp>

using namespace sf1r;

TopKReranker::TopKReranker(SearchManagerPreProcessor& preprocessor)
    : preprocessor_(preprocessor)
    , productRankerFactory_(NULL)
{
}

void TopKReranker::setProductRankerFactory(ProductRankerFactory* productRankerFactory)
{
    productRankerFactory_ = productRankerFactory;
}

bool TopKReranker::rerank(
    const KeywordSearchActionItem& actionItem,
    KeywordSearchResult& resultItem)
{
    if (productRankerFactory_ &&
        resultItem.topKCustomRankScoreList_.empty() &&
        preprocessor_.isNeedRerank(actionItem))
    {
        izenelib::util::ClockTimer timer;

        ProductRankParam rankParam(resultItem.topKDocs_,
                                   resultItem.topKRankScoreList_,
                                   actionItem.isRandomRank_);

        boost::scoped_ptr<ProductRanker> productRanker(
            productRankerFactory_->createProductRanker(rankParam));

        productRanker->rank();

        LOG(INFO) << "topK doc num: " << resultItem.topKDocs_.size()
                  << ", product rerank costs: " << timer.elapsed()
                  << " seconds";

        return true;
    }

    return false;
}
