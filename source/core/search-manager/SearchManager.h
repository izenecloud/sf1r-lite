#ifndef CORE_SEARCH_MANAGER_SEARCH_MANAGER_H
#define CORE_SEARCH_MANAGER_SEARCH_MANAGER_H

#include <configuration-manager/PropertyConfig.h>
#include <query-manager/SearchKeywordOperation.h>
#include <query-manager/ActionItem.h>
#include <ranking-manager/RankQueryProperty.h>
#include <common/ResultType.h>
#include "NumericPropertyTable.h"
#include "NumericPropertyTableBuilder.h"
#include "ANDDocumentIterator.h"
#include "Sorter.h"

#include <ir/id_manager/IDManager.h>

#include <util/ustring/UString.h>

#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <boost/function.hpp>

#include <vector>
#include <deque>
#include <set>


namespace sf1r
{
class IndexSearchCache;
class PureQueryIdentity;
class QueryBuilder;
class DocumentManager;
class RankingManager;
class IndexManager;
class MiningManager;
class Sorter;
class IndexBundleConfiguration;
class PropertyRanker;
class MultiPropertyScorer;
class CombinedDocumentIterator;
class HitQueue;

namespace faceted
{
class GroupFilterBuilder;
class OntologyRep;
class GroupFilter;
}

class SearchManager : public NumericPropertyTableBuilder
{
    typedef izenelib::ir::idmanager::IDManager IDManager;
    enum IndexLevel
    {
        DOCLEVEL,  /// position posting does not create
        WORDLEVEL ///  position postings create
    };
    typedef std::map<std::string, PropertyTermInfo> property_term_info_map;

public:
    typedef boost::function< void( std::vector<unsigned int>&, std::vector<float>&, const std::string& ) > reranker_t;
    typedef boost::function< void( std::vector<QueryFiltering::FilteringType>& ) > filter_hook_t;

    SearchManager(
            const IndexBundleSchema& indexSchema,
            const boost::shared_ptr<IDManager>& idManager,
            const boost::shared_ptr<DocumentManager>& documentManager,
            const boost::shared_ptr<IndexManager>& indexManager,
            const boost::shared_ptr<RankingManager>& rankingManager,
            IndexBundleConfiguration* config);

    ~SearchManager();

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
            uint32_t topK = 200,
            uint32_t knnTopK = 200,
            uint32_t knnDist = 15,
            uint32_t start = 0);

    bool rerank(const KeywordSearchActionItem& actionItem, KeywordSearchResult& resultItem);

    void reset_cache(
            bool rType,
            docid_t id,
            const std::map<std::string, pair<PropertyDataType, izenelib::util::UString> >& rTypeFieldValue);

    void reset_all_property_cache();

    void set_reranker(reranker_t reranker)
    {
        reranker_ = reranker;
    }

    void set_filter_hook(filter_hook_t filter_hook)
    {
        filter_hook_ = filter_hook;
    }

    void setGroupFilterBuilder(faceted::GroupFilterBuilder* builder);

    void setMiningManager(boost::shared_ptr<MiningManager> miningManagerPtr);

    NumericPropertyTable* createPropertyTable(const std::string& propertyName);

private:
    bool doSearch_(
            SearchKeywordOperation& actionOperation,
            std::vector<unsigned int>& docIdList,
            std::vector<float>& rankScoreList,
            std::vector<float>& customRankScoreList,
            std::size_t& totalCount,
            sf1r::PropertyRange& propertyRange,
            uint32_t start,
            std::vector<RankQueryProperty>& rankQueryProperties,
            std::vector<boost::shared_ptr<PropertyRanker> >& propertyRankers,
            Sorter* pSorter,
            CustomRankerPtr customRanker,
            MultiPropertyScorer* pMultiPropertyIterator,
            CombinedDocumentIterator* pDocIterator,
            faceted::GroupFilter* groupFilter,
            HitQueue* scoreItemQueue);

    void post_prepare_ranker_(
            const std::vector<std::string>& indexPropertyList,
            unsigned indexPropertySize,
            const property_term_info_map& propertyTermInfoMap,
            DocumentFrequencyInProperties& dfmap,
            CollectionTermFrequencyInProperties& ctfmap,
            bool readTermPosition,
            std::vector<RankQueryProperty>& rankQueryProperties,
            std::vector<boost::shared_ptr<PropertyRanker> >& propertyRankers);


    void prepare_sorter_customranker_(
            SearchKeywordOperation& actionOperation,	
            CustomRankerPtr& customRanker, 
            Sorter* &pSorter);

    bool prepareDocIterWithOnlyOrderby_(
            boost::shared_ptr<EWAHBoolArray<uint32_t> >& pFilterIdSet);

    /**
     * @brief get corresponding id of the property, returns 0 if the property
     * does not exist.
     * @see CollectionMeta::numberPropertyConfig
     */
    propertyid_t getPropertyIdByName_(const std::string& name) const;

    bool getPropertyTypeByName_(
            const std::string& name,
            PropertyDataType& type) const;

    boost::shared_ptr<PropertyData> getPropertyData_(const std::string& name);

    /**
     * rebuild custom ranker.
     * @param actionItem
     * @return
     */
    CustomRankerPtr buildCustomRanker_(KeywordSearchActionItem& actionItem);

    /**
     * @brief get data list of each sort property for documents referred by docIdList,
     * used in distributed search for merging topk results.
     * @param pSorter [OUT]
     * @param docIdList [IN]
     * @param distSearchInfo [OUT]
     */
    void getSortPropertyData_(
            Sorter* pSorter,
            std::vector<unsigned int>& docIdList,
            DistKeywordSearchInfo& distSearchInfo);

    void makeQueryIdentity(
            PureQueryIdentity& identity,
            const KeywordSearchActionItem& item,
            uint32_t start = 0);


    /**
     * @brief for testing
     */
    void printDFCTF_(
            DocumentFrequencyInProperties& dfmap,
            CollectionTermFrequencyInProperties ctfmap);

private:
    std::string collectionName_;
    boost::unordered_map<std::string, PropertyConfig> schemaMap_;
    boost::shared_ptr<IndexManager> indexManagerPtr_;
    boost::shared_ptr<DocumentManager> documentManagerPtr_;
    boost::shared_ptr<RankingManager> rankingManagerPtr_;
    boost::weak_ptr<MiningManager> miningManagerPtr_;
    boost::shared_ptr<QueryBuilder> queryBuilder_;
    std::map<propertyid_t, float> propertyWeightMap_;
    boost::scoped_ptr<IndexSearchCache> cache_;
    SortPropertyCache* pSorterCache_;

    filter_hook_t filter_hook_;

    reranker_t reranker_;

    boost::scoped_ptr<faceted::GroupFilterBuilder> groupFilterBuilder_;
};

} // end - namespace sf1r

#endif // CORE_SEARCH_MANAGER_SEARCH_MANAGER_H

