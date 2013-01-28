#include "NormalSearch.h"
#include "SearchThreadParam.h"
#include <mining-manager/MiningManager.h>

using namespace sf1r;

NormalSearch::NormalSearch(
    const IndexBundleConfiguration& config,
    const boost::shared_ptr<DocumentManager>& documentManager,
    const boost::shared_ptr<IndexManager>& indexManager,
    const boost::shared_ptr<RankingManager>& rankingManager,
    SearchManagerPreProcessor& preprocessor,
    QueryBuilder& queryBuilder)
    : searchThreadWorker_(config, documentManager, indexManager, rankingManager,
                          preprocessor, queryBuilder)
    , searchThreadMaster_(config, documentManager,
                          preprocessor, searchThreadWorker_)
{
}

void NormalSearch::setMiningManager(
    const boost::shared_ptr<MiningManager>& miningManager)
{
    searchThreadWorker_.setGroupFilterBuilder(
        miningManager->GetGroupFilterBuilder());

    searchThreadWorker_.setCustomRankManager(
        miningManager->GetCustomRankManager());
}

bool NormalSearch::search(
    const SearchKeywordOperation& actionOperation,
    KeywordSearchResult& searchResult,
    std::size_t limit,
    std::size_t offset)
{
    if (!actionOperation.noError())
        return false;

    const std::size_t heapSize = limit + offset;
    std::vector<SearchThreadParam> threadParams;
    DistKeywordSearchInfo& distSearchInfo = searchResult.distSearchInfo_;

    searchThreadMaster_.prepareThreadParams(actionOperation,
                                            distSearchInfo,
                                            heapSize,
                                            threadParams);

    if (!searchThreadMaster_.runThreadParams(threadParams))
        return false;

    if (distSearchInfo.isOptionGatherInfo())
        return true;

    bool result = searchThreadMaster_.mergeThreadParams(threadParams) &&
                  searchThreadMaster_.fetchSearchResult(offset,
                                                        threadParams.front(),
                                                        searchResult);
    return result;
}
