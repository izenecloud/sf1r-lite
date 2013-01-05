#ifndef CORE_SEARCH_MANAGER_SEARCH_MANAGER_H
#define CORE_SEARCH_MANAGER_SEARCH_MANAGER_H

#include <configuration-manager/PropertyConfig.h>
#include <query-manager/SearchKeywordOperation.h>
#include <query-manager/ActionItem.h>
#include <ranking-manager/RankQueryProperty.h>
#include <common/ResultType.h>
#include "ANDDocumentIterator.h"
#include "Sorter.h"
#include "NumericPropertyTableBuilder.h"

#include <ir/id_manager/IDManager.h>

#include <util/ustring/UString.h>

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <boost/function.hpp>
#include <boost/threadpool.hpp>

#include <vector>
#include <deque>
#include <set>

using izenelib::ir::idmanager::IDManager;

namespace sf1r
{

class QueryBuilder;
class DocumentManager;
class RankingManager;
class IndexManager;
class MiningManager;
class Sorter;
class IndexBundleConfiguration;
class PropertyRanker;
class MultiPropertyScorer;
class WANDDocumentIterator;
class CombinedDocumentIterator;
class HitQueue;
struct SearchThreadParam;
class SearchManagerPreProcessor;
class CustomRankManager;
class ScoreDocEvaluator;
class ProductScorerFactory;
class ProductRankerFactory;
class PropSharedLockSet;

namespace faceted
{
class GroupFilterBuilder;
class OntologyRep;
class GroupFilter;
}

class SearchManager : public NumericPropertyTableBuilder
{
    enum IndexLevel
    {
        DOCLEVEL,  /// position posting does not create
        WORDLEVEL ///  position postings create
    };
    typedef std::map<std::string, PropertyTermInfo> property_term_info_map;

public:
    typedef boost::function< void( std::vector<QueryFiltering::FilteringType>& ) > filter_hook_t;

    SearchManager(
        const IndexBundleSchema& indexSchema,
        const boost::shared_ptr<IDManager>& idManager,
        const boost::shared_ptr<DocumentManager>& documentManager,
        const boost::shared_ptr<IndexManager>& indexManager,
        const boost::shared_ptr<RankingManager>& rankingManager,
        IndexBundleConfiguration* config);

    ~SearchManager();

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

    void reset_all_property_cache();

    void reset_filter_cache();

    void set_filter_hook(filter_hook_t filter_hook)
    {
        filter_hook_ = filter_hook;
    }

    void setGroupFilterBuilder(faceted::GroupFilterBuilder* builder);

    void setMiningManager(boost::shared_ptr<MiningManager> miningManagerPtr);

    boost::shared_ptr<NumericPropertyTableBase>& createPropertyTable(const std::string& propertyName);

    void setCustomRankManager(CustomRankManager* customRankManager);

    void setProductScorerFactory(ProductScorerFactory* productScorerFactory);

    void setProductRankerFactory(ProductRankerFactory* productRankerFactory);

    QueryBuilder* getQueryBuilder()
    {
        return queryBuilder_.get();
    }

private:
    void prepareThreadParams_(
        const SearchKeywordOperation& actionOperation,
        DistKeywordSearchInfo& distSearchInfo,
        std::size_t heapSize,
        std::vector<SearchThreadParam>& threadParams);

    void getThreadInfo_(
        const DistKeywordSearchInfo& distSearchInfo,
        std::size_t& threadNum,
        std::size_t& runningNode);

    bool runThreadParams_(
        std::vector<SearchThreadParam>& threadParams);

    bool runSingleThread_(
        SearchThreadParam& threadParam);

    bool runMultiThreads_(
        std::vector<SearchThreadParam>& threadParams);

    bool mergeThreadParams_(
        std::vector<SearchThreadParam>& threadParams) const;

    bool fetchSearchResult_(
        std::size_t offset,
        SearchThreadParam& threadParam,
        KeywordSearchResult& searchResult);

    bool doSearch_(
        SearchThreadParam& pParam,
        CombinedDocumentIterator* pDocIterator,
        faceted::GroupFilter* groupFilter,
        ScoreDocEvaluator& scoreDocEvaluator,
        PropSharedLockSet& propSharedLockSet);

    void doSearchInThreadOneParam(
        SearchThreadParam* pParam,
        boost::detail::atomic_count* finishedJobs);

    bool doSearchInThread(SearchThreadParam& pParam);

    void prepare_sorter_customranker_(
        const SearchKeywordOperation& actionOperation,
        CustomRankerPtr& customRanker,
        boost::shared_ptr<Sorter> &pSorter);

    /**
     * @brief get data list of each sort property for documents referred by docIdList,
     * used in distributed search for merging topk results.
     * @param pSorter [OUT]
     * @param docIdList [IN]
     * @param distSearchInfo [OUT]
     */
    void fillSearchInfoWithSortPropertyData_(
        Sorter* pSorter,
        std::vector<unsigned int>& docIdList,
        DistKeywordSearchInfo& distSearchInfo);

    /**
     * combine the @p originDocIterator with the customized doc iterator.
     * @return the combined doc iterator instance, it would be just
     *         @p originDocIterator if no customized doc iterator is created.
     */
    DocumentIterator* combineCustomDocIterator_(
        const KeywordSearchActionItem& actionItem,
        DocumentIterator* originDocIterator);

    score_t getFuzzyScoreWeight_() const;

private:
    /**
     * @brief for testing
     */
    void printDFCTF_(
        DocumentFrequencyInProperties& dfmap,
        CollectionTermFrequencyInProperties ctfmap);

private:
    IndexBundleConfiguration* config_;
    const bool isParallelEnabled_;
    std::string collectionName_;
    boost::shared_ptr<IndexManager> indexManagerPtr_;
    boost::shared_ptr<DocumentManager> documentManagerPtr_;
    boost::shared_ptr<RankingManager> rankingManagerPtr_;
    boost::weak_ptr<MiningManager> miningManagerPtr_;
    boost::scoped_ptr<QueryBuilder> queryBuilder_;
    std::map<propertyid_t, float> propertyWeightMap_;

    SortPropertyCache* pSorterCache_;

    filter_hook_t filter_hook_;

    boost::scoped_ptr<faceted::GroupFilterBuilder> groupFilterBuilder_;
    CustomRankManager* customRankManager_;

    boost::threadpool::pool  threadpool_;
    SearchManagerPreProcessor*  preprocessor_;

    ProductRankerFactory* productRankerFactory_;
};

} // end - namespace sf1r

#endif // CORE_SEARCH_MANAGER_SEARCH_MANAGER_H
