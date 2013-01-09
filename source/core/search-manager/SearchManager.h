#ifndef CORE_SEARCH_MANAGER_SEARCH_MANAGER_H
#define CORE_SEARCH_MANAGER_SEARCH_MANAGER_H

#include "QueryBuilder.h"
#include "SearchManagerPreProcessor.h"
#include "SearchThreadWorker.h"
#include "SearchThreadMaster.h"
#include <configuration-manager/PropertyConfig.h>
#include <ir/id_manager/IDManager.h>

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <vector>

using izenelib::ir::idmanager::IDManager;

namespace sf1r
{
class IndexBundleConfiguration;
class SearchKeywordOperation;
class KeywordSearchActionItem;
class KeywordSearchResult;
class DocumentManager;
class RankingManager;
class IndexManager;
class MiningManager;
class CustomRankManager;
class ProductScorerFactory;
class ProductRankerFactory;
class NumericPropertyTableBuilder;

namespace faceted { class GroupFilterBuilder; }

class SearchManager
{
public:
    SearchManager(
        const IndexBundleSchema& indexSchema,
        const boost::shared_ptr<IDManager>& idManager,
        const boost::shared_ptr<DocumentManager>& documentManager,
        const boost::shared_ptr<IndexManager>& indexManager,
        const boost::shared_ptr<RankingManager>& rankingManager,
        IndexBundleConfiguration* config);

    void rankDocIdListForFuzzySearch(const SearchKeywordOperation& actionOperation,
                                     uint32_t start, std::vector<uint32_t>& docid_list, std::vector<float>& result_score_list,
                                     std::vector<float>& custom_score_list);

    /**
     * get search results.
     *
     * @param actionOperation the search parameters
     * @param searchResult it stores the search results
     * @param limit at most how many docs to return. Generally it's TOP_K_NUM.
     * @param offset the index offset of the first returned doc in all candidate
     *        docs, start from 0. Generally it's a multiple of TOP_K_NUM.
     *
     * @note in SF1R driver API documents/search(), there are also such
     *       parameters as "offset" and "limit", which are used for pagination.
     *
     *       While the @p offset and @p limit of this method have different
     *       meanings, they are used to get a batch of docs.
     *
     *       By putting the batch into cache, there is no need to call this
     *       method again as long as the cache is hit.
     */
    bool search(
        const SearchKeywordOperation& actionOperation,
        KeywordSearchResult& searchResult,
        std::size_t limit,
        std::size_t offset);

    bool rerank(
        const KeywordSearchActionItem& actionItem,
        KeywordSearchResult& resultItem);

    void reset_filter_cache();

    void setGroupFilterBuilder(faceted::GroupFilterBuilder* builder);

    void setMiningManager(boost::shared_ptr<MiningManager> miningManagerPtr);

    void setCustomRankManager(CustomRankManager* customRankManager);

    void setProductScorerFactory(ProductScorerFactory* productScorerFactory);

    void setNumericTableBuilder(NumericPropertyTableBuilder* numericTableBuilder);

    void setProductRankerFactory(ProductRankerFactory* productRankerFactory);

    QueryBuilder* getQueryBuilder()
    {
        return queryBuilder_.get();
    }

private:
    score_t getFuzzyScoreWeight_() const;

private:
    SearchManagerPreProcessor preprocessor_;

    boost::scoped_ptr<QueryBuilder> queryBuilder_;
    boost::scoped_ptr<SearchThreadWorker> searchThreadWorker_;
    boost::scoped_ptr<SearchThreadMaster> searchThreadMaster_;

    boost::weak_ptr<MiningManager> miningManagerPtr_;
    ProductRankerFactory* productRankerFactory_;
};

} // end - namespace sf1r

#endif // CORE_SEARCH_MANAGER_SEARCH_MANAGER_H
