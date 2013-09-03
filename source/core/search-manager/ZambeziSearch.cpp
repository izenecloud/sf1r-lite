#include "ZambeziSearch.h"
#include <query-manager/SearchKeywordOperation.h>
#include <common/ResultType.h>
#include <common/ResourceManager.h>
#include <mining-manager/MiningManager.h>
#include <mining-manager/zambezi-manager/ZambeziManager.h>
#include <glog/logging.h>
#include <iostream>

namespace
{
const std::size_t kZambeziTopKNum = 1e6;
}

using namespace sf1r;

ZambeziSearch::ZambeziSearch(SearchManagerPreProcessor& preprocessor)
    : preprocessor_(preprocessor)
    , zambeziManager_(NULL)
{
}

void ZambeziSearch::setMiningManager(
    const boost::shared_ptr<MiningManager>& miningManager)
{
    miningManager_ = miningManager;
    zambeziManager_ = miningManager->getZambeziManager();
}

bool ZambeziSearch::search(
    const SearchKeywordOperation& actionOperation,
    KeywordSearchResult& searchResult,
    std::size_t limit,
    std::size_t offset)
{
    const std::string& query = actionOperation.actionItem_.env_.queryString_;
    LOG(INFO) << "zambezi search for query: " << query << endl;

    if (query.empty())
        return false;

    std::vector<docid_t> docIds;
    std::vector<float> scores;

    if (!getTopKDocs_(query, docIds, scores))
        return false;

    return true;
}

bool ZambeziSearch::getTopKDocs_(
    const std::string& query,
    std::vector<docid_t>& docIds,
    std::vector<float>& scores)
{
    if (!zambeziManager_)
    {
        LOG(WARNING) << "the instance of ZambeziManager is empty";
        return false;
    }

    KNlpWrapper::token_score_list_t tokenScores;
    KNlpWrapper::string_t kstr(query);
    boost::shared_ptr<KNlpWrapper> knlpWrapper = KNlpResourceManager::getResource();
    knlpWrapper->fmmTokenize(kstr, tokenScores);

    std::vector<std::string> tokenList;
    std::cout << "tokens:" << std::endl;

    for (KNlpWrapper::token_score_list_t::const_iterator it =
             tokenScores.begin(); it != tokenScores.end(); ++it)
    {
        std::string token = it->first.get_bytes("utf-8");
        tokenList.push_back(token);
        std::cout << token << std::endl;
    }
    std::cout << "-----" << std::endl;

    zambeziManager_->search(tokenList, kZambeziTopKNum, docIds, scores);

    if (docIds.empty())
    {
        LOG(INFO) << "empty search result for query: " << query << endl;
        return false;
    }

    if (docIds.size() != scores.size())
    {
        LOG(WARNING) << "mismatch size of docid and score";
        return false;
    }

    return true;
}
