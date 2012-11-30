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
class SearchThreadParam;
class SearchManagerPreProcessor;
class CustomRankManager;
class ScoreDocEvaluator;
class ProductScorerFactory;

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

    bool search(
            SearchKeywordOperation& actionOperation,
            std::vector<unsigned int>& docIdList,
            std::vector<float>& rankScoreList,
            std::vector<float>& customRankScoreList,
            std::size_t& totalCount,
            faceted::GroupRep& groupRep,
            faceted::OntologyRep& attrRep,
            sf1r::PropertyRange& propertyRange,
            DistKeywordSearchInfo& distSearchInfo,
            std::map<std::string, unsigned int>& counterResults,
            uint32_t topK = 200,
            uint32_t knnTopK = 200,
            uint32_t knnDist = 15,
            uint32_t start = 0,
            bool enable_parallel_searching = false);

    void reset_all_property_cache();

    void set_filter_hook(filter_hook_t filter_hook)
    {
        filter_hook_ = filter_hook;
    }

    void setGroupFilterBuilder(faceted::GroupFilterBuilder* builder);

    void setMiningManager(boost::shared_ptr<MiningManager> miningManagerPtr);

    boost::shared_ptr<NumericPropertyTableBase>& createPropertyTable(const std::string& propertyName);

    void setCustomRankManager(CustomRankManager* customRankManager);

    void setProductScorerFactory(ProductScorerFactory* productScorerFactory);

    QueryBuilder* getQueryBuilder() { return queryBuilder_.get(); }

private:
    bool doSearch_(
            const SearchKeywordOperation& actionOperation,
            std::size_t& totalCount,
            sf1r::PropertyRange& propertyRange,
            CombinedDocumentIterator* pDocIterator,
            faceted::GroupFilter* groupFilter,
            ScoreDocEvaluator& scoreDocEvaluator,
            HitQueue* scoreItemQueue,
            std::map<std::string, unsigned int>& counterResults,
            std::size_t docid_start,
            std::size_t docid_num_byeachthread,
            std::size_t docid_nextstart_inc);

    void doSearchInThreadOneParam(SearchThreadParam* pParam,
            boost::detail::atomic_count* finishedJobs);

    bool doSearchInThread(const SearchKeywordOperation& actionOperation,
            std::size_t& totalCount,
            sf1r::PropertyRange& propertyRange,
            uint32_t start,
            boost::shared_ptr<Sorter>& pSorter_orig,
            CustomRankerPtr& customRanker_orig,
            faceted::GroupRep& groupRep,
            faceted::OntologyRep& attrRep,
            boost::shared_ptr<HitQueue>& scoreItemQueue,
            DistKeywordSearchInfo& distSearchInfo,
            std::map<std::string, unsigned int>& counterResults,
            int heapSize,
            std::size_t docid_start,
            std::size_t docid_num_byeachthread,
            std::size_t docid_nextstart_inc,
            bool is_parallel = false);

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
     * @param query the user query
     * @param pSorter the Sorter instance, it decides whether need to create
     *                the customized doc iterator.
     * @return the combined doc iterator instance, it would be just
     *         @p originDocIterator if no customized doc iterator is created.
     */
    DocumentIterator* combineCustomDocIterator_(
        const std::string& query,
        boost::shared_ptr<Sorter> pSorter,
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
};

} // end - namespace sf1r

#endif // CORE_SEARCH_MANAGER_SEARCH_MANAGER_H
