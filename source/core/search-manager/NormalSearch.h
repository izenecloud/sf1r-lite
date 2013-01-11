/**
 * @file NormalSearch.h
 * @brief it implements search using the normal inverted index.
 * @author Jun Jiang
 * @date Created 2013-01-11
 */

#ifndef SF1R_NORMAL_SEARCH_H
#define SF1R_NORMAL_SEARCH_H

#include "SearchBase.h"
#include "SearchThreadWorker.h"
#include "SearchThreadMaster.h"

namespace sf1r
{

class NormalSearch : public SearchBase
{
public:
    NormalSearch(
        const IndexBundleConfiguration& config,
        const boost::shared_ptr<DocumentManager>& documentManager,
        const boost::shared_ptr<IndexManager>& indexManager,
        const boost::shared_ptr<RankingManager>& rankingManager,
        SearchManagerPreProcessor& preprocessor,
        QueryBuilder& queryBuilder);

    virtual void setMiningManager(
        const boost::shared_ptr<MiningManager>& miningManager);

    virtual bool search(
        const SearchKeywordOperation& actionOperation,
        KeywordSearchResult& searchResult,
        std::size_t limit,
        std::size_t offset);

private:
    SearchThreadWorker searchThreadWorker_;
    SearchThreadMaster searchThreadMaster_;
};

} // namespace sf1r

#endif // SF1R_NORMAL_SEARCH_H
