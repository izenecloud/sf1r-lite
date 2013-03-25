/// @file MiningManager.h
/// @date 2009-08-03
/// @author Jia Guo&&Jinglei Zhao
/// @brief This class implements the MiningManager that is an interface to a set of subcomponents with various functionalities
///
/// @details
/// - Log
///   Move RawMapworker from SIA to MIA, so that MIA is indepentant from SIA.
///   -- by Peisheng Wang 2009-11-10
///
///   Refined on: 2009-11-25
///       Author: Jinglei


#ifndef SF1R_MINING_MANAGER_H_
#define SF1R_MINING_MANAGER_H_

#include "group-manager/PropValueTable.h" // pvid_t
#include "summarization-submanager/Summarization.h" //Summarization
#include "custom-rank-manager/CustomRankValue.h"
#include "merchant-score-manager/MerchantScore.h"

#include <common/ResultType.h>
#include <configuration-manager/PropertyConfig.h>
#include <query-manager/SearchKeywordOperation.h>
#include <configuration-manager/MiningConfig.h>
#include <configuration-manager/MiningSchema.h>
#include <configuration-manager/CollectionPath.h>
#include <ir/id_manager/IDManager.h>
#include <boost/atomic.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/thread/mutex.hpp>
#include <util/PriorityQueue.h>
#include <string>
#include <map>


namespace izenelib
{
namespace ir
{
namespace indexmanager
{
class IndexReader;
}

}
}

namespace idmlib
{
namespace tdt
{
class Storage;
class TrackResult;
}

namespace sim
{
template <typename T> class TermSimilarityTable;
template <class T> class SimOutputCollector;
template <class T, typename A, typename B, typename C> class TermSimilarity;
class DocSimOutput;
}


namespace util
{
template <class T> class ContainerSwitch;
class IDMAnalyzer;
}

}


namespace sf1r
{

class DupDetectorWrapper;
class GroupLabelLogger;
class QueryRecommendSubmanager;
class QueryRecommendRep;
class RecommendManager;
class QueryCorrectionSubmanager;
class LabelManager;
class TaxonomyGenerationSubManager;
class TaxonomyInfo;
class Document;
class DocumentManager;
class LAManager;
class IndexManager;
class SearchManager;
class SearchCache;
class MultiDocSummarizationSubManager;
class MerchantScoreManager;
class CustomRankManager;
class CustomDocIdConverter;
class ProductScorerFactory;
class ProductScoreManager;
class OfflineProductScorerFactory;
class ProductRankerFactory;
class NaiveTopicDetector;
class SuffixMatchManager;
class IncrementalManager;
class ProductMatcher;
class QueryCategorizer;
class MiningTaskBuilder;
class GroupLabelKnowledge;
class NumericPropertyTableBuilder;

namespace sim
{
class SimilarityIndex;
}

namespace faceted
{
class GroupManager;
class AttrManager;
class OntologyManager;
class CTRManager;
class GroupFilterBuilder;
}

/**
 * @brief MiningManager manages all the data mining tasks.
 * Those tasks needs heavy offline mining computation. Also, it manages the online query to
 * to the mining results.  Currently, it includes  taxonomy generation, duplication detection,
 * query recommendation and similarity clustering.
 */
class MiningManager
{
typedef DupDetectorWrapper DupDType;
typedef std::pair<double, uint32_t> ResultT;
typedef idmlib::util::ContainerSwitch<idmlib::tdt::Storage> TdtStorageType;
typedef idmlib::sim::TermSimilarityTable<uint32_t> SimTableType;
typedef idmlib::sim::SimOutputCollector<SimTableType> SimCollectorType;
typedef idmlib::sim::TermSimilarity<SimCollectorType, uint32_t, uint32_t, uint32_t> TermSimilarityType;
typedef izenelib::am::leveldb::Table<std::string, std::string> KVSubManager;
public:
    /**
     * @brief The constructor of MiningManager.
     * @param miningManagerConfig specifies the user configured parameters from configuration file.
     * @param idManager used for generating term Ids.
     * @param docIdManager used for generating docIds in parsing the SCD file.
     */
    MiningManager(const std::string& collectionDataPath,
                  const CollectionPath& collectionPath,
                  const boost::shared_ptr<DocumentManager>& documentManager,
                  const boost::shared_ptr<LAManager>& laManager,
                  const boost::shared_ptr<IndexManager>& index_manager,
                  const boost::shared_ptr<SearchManager>& searchManager,
                  const boost::shared_ptr<SearchCache>& searchCache,
                  const boost::shared_ptr<izenelib::ir::idmanager::IDManager>& idManager,
                  const std::string& collectionName,
                  const DocumentSchema& documentSchema,
                  const MiningConfig& miningConfig,
                  const MiningSchema& miningSchema,
                  const IndexBundleSchema& indexSchema);

    ~MiningManager();

    bool open();

    void flush();

    bool DoMiningCollection();

    bool DOMiningTask();

    void DoContinue();

    void DoSyncFullSummScd();

    const MiningSchema& getMiningSchema() const { return mining_schema_; }

    /**
     * @brief The online querying interface.
     */
    bool getMiningResult(const KeywordSearchActionItem& actionItem, KeywordSearchResult& miaInput);

    /**
     * @brief Gets list of documents with images similar to the target image.
     */
    bool getSimilarImageDocIdList(
        const std::string& targetImageURI,
        SimilarImageDocIdList& imageDocIdList
    );


    bool getReminderQuery(std::vector<izenelib::util::UString>& popularQueries,
                          std::vector<izenelib::util::UString>& realtimeQueries);
    /**
     * @brief Get the unique document list that eliminates duplicates from the result.
     * For example, given @p docIdList "1 2 3 4 5 6", and "2 4 6" are duplicated
     * docs, then @p cleanDocs would be "1 3 5".
     */
    bool getUniqueDocIdList(
        const std::vector<uint32_t>& docIdList,
        std::vector<uint32_t>& cleanDocs);

    /**
     * @brief Get the unique docid positions in original list.
     * For example, given @p docIdList "1 2 3 4 5 6", and "2 4 6" are duplicated
     * docs, then @p uniquePosList would be "0 2 4".
     */
    bool getUniquePosList(
        const std::vector<uint32_t>& docIdList,
        std::vector<std::size_t>& uniquePosList);

    /**
     * @brief Get the duplicated documents for a given document.
     * @param docId the given document
     * @param docIdList the duplicates of this document.
     */
    bool getDuplicateDocIdList(uint32_t docId,
                               std::vector<uint32_t>& docIdList);


    /**
     * @brief The online querying interface for similarity clustering.
     * @param documentId the given document to get its similar docs.
     * @param MaxNum the maximum number of similar documents returned.
     * @param result the resulted similar docs.
     */
    bool getSimilarDocIdList(uint32_t documentId, uint32_t maxNum, std::vector<
                             std::pair<uint32_t, float> >& result);


    bool getSimilarLabelList(uint32_t label_id, std::vector<uint32_t>& sim_list);

    bool getSimilarLabelStringList(uint32_t label_id, std::vector<izenelib::util::UString>& sim_list);

    bool getLabelListByDocId(uint32_t docid, std::vector<std::pair<uint32_t, izenelib::util::UString> >& label_list);

    bool getLabelListWithSimByDocId(uint32_t docid,
                                    std::vector<std::pair<izenelib::util::UString, std::vector<izenelib::util::UString> > >& label_list);

    /**
     * Visit document, for updatint click-through rate (CTR).
     * @param docId spedified document id
     * @return true if success, or false;
     */
    bool visitDoc(uint32_t docId);

    /**
     * Log the group label click.
     * @param query user query
     * @param propName the property name of the group label
     * @param groupPath the path of the group label
     * @return true for success, false for failure
     */
    bool clickGroupLabel(
            const std::string& query,
            const std::string& propName,
            const std::vector<std::string>& groupPath
    );

    /**
     * Get the most frequently clicked group labels.
     * @param query user query
     * @param propName the property name for the group labels to get
     * @param limit the max number of labels to get
     * @param pathVec store the path for each group label
     * @param freqVec the click count for each group label
     * @return true for success, false for failure
     * @post @p freqVec is sorted in descending order.
     */
    bool getFreqGroupLabel(
            const std::string& query,
            const std::string& propName,
            int limit,
            std::vector<std::vector<std::string> >& pathVec,
            std::vector<int>& freqVec
    );

    /**
     * Set the most frequently clicked group label.
     * @param query user query
     * @param propName the property name
     * @param groupLabels an array of group paths
     * @return true for success, false for failure
     */
    bool setTopGroupLabel(
            const std::string& query,
            const std::string& propName,
            const std::vector<std::vector<std::string> >& groupLabels
    );

    /**
     * if @p merchantNames is empty, it loads all existing merchant score into @p merchantScoreMap;
     * otherwise, it loads the score of merchant in @p merchantNames into @p merchantScoreMap.
     * @return true for success, false for failure
     */
    bool getMerchantScore(
        const std::vector<std::string>& merchantNames,
        MerchantStrScoreMap& merchantScoreMap
    ) const;

    /**
     * set merchant score according to @p strScoreMap.
     * @return true for success, false for failure
     */
    bool setMerchantScore(const MerchantStrScoreMap& merchantScoreMap);

    bool setCustomRank(
        const std::string& query,
        const CustomRankDocStr& customDocStr
    );

    bool getCustomRank(
        const std::string& query,
        std::vector<Document>& topDocList,
        std::vector<Document>& excludeDocList
    );

    bool getCustomQueries(std::vector<std::string>& queries);

    /**
     * get product score.
     * @param docIdStr the doc id
     * @param scoreTypeName such as "popularity", "_ctr"
     * @param scoreValue the score value as output
     * @return true for success, false for failure
     */
    bool getProductScore(
        const std::string& docIdStr,
        const std::string& scoreTypeName,
        score_t& scoreValue);

    bool GetTdtInTimeRange(const izenelib::util::UString& start, const izenelib::util::UString& end, std::vector<izenelib::util::UString>& topic_list);

    bool GetTdtInTimeRange(const boost::gregorian::date& start, const boost::gregorian::date& end, std::vector<izenelib::util::UString>& topic_list);

    bool GetTdtTopicInfo(const izenelib::util::UString& text, std::pair<idmlib::tdt::TrackResult, std::vector<izenelib::util::UString> >& info);

    bool GetTopics(const std::string& content, std::vector<std::string>& topic_list, size_t limit);

    void GetRefinedQuery(const izenelib::util::UString& query, izenelib::util::UString& result);

    void InjectQueryCorrection(const izenelib::util::UString& query, const izenelib::util::UString& result);

    void FinishQueryCorrectionInject();

    void InjectQueryRecommend(const izenelib::util::UString& query, const izenelib::util::UString& result);

    void FinishQueryRecommendInject();

    bool GetSummarizationByRawKey(const izenelib::util::UString& rawKey, Summarization& result);

    uint32_t GetSignatureForQuery(
            const KeywordSearchActionItem& item,
            std::vector<uint64_t>& signature);

    bool GetKNNListBySignature(
            const std::vector<uint64_t>& signature,
            std::vector<uint32_t>& docIdList,
            std::vector<float>& rankScoreList,
            std::size_t& totalCount,
            uint32_t knnTopK,
            uint32_t knnDist,
            uint32_t start);

    void EnsureHasDeletedDocDuringMining() { hasDeletedDocDuringMining_ = true; }

    bool HasDeletedDocDuringMining() { return hasDeletedDocDuringMining_; }

    bool GetSuffixMatch(
            const SearchKeywordOperation& actionOperation,
            uint32_t max_docs,
            bool use_fuzzy,
            uint32_t start,
            const std::vector<QueryFiltering::FilteringType>& filter_param,
            std::vector<uint32_t>& docIdList,
            std::vector<float>& rankScoreList,
            std::vector<float>& customRankScoreList,
            std::size_t& totalCount,
            faceted::GroupRep& groupRep,
            sf1r::faceted::OntologyRep& attrRep,
            UString& analyzedQuery
            );

    bool GetProductCategory(const std::string& squery, int limit, std::vector<std::vector<std::string> >& pathVec );

    bool GetProductFrontendCategory(const izenelib::util::UString& query, int limit, std::vector<UString>& frontends);
    bool GetProductFrontendCategory(const izenelib::util::UString& query, UString& frontend);
    bool GetProductCategory(const UString& query, UString& backend);

    bool SetKV(const std::string& key, const std::string& value);

    bool GetKV(const std::string& key, std::string& value);

    void close();

    boost::shared_ptr<faceted::OntologyManager>& GetFaceted()
    {
        return faceted_;
    }

    boost::shared_ptr<TaxonomyGenerationSubManager>& GetTgManager()
    {
        return tgManager_;
    }

    boost::shared_ptr<DocumentManager>& GetDocumentManager()
    {
        return  document_manager_;
    }

    boost::shared_ptr<RecommendManager>& GetRecommendManager()
    {
        return rmDb_;
    }

    boost::shared_ptr<faceted::CTRManager>& GetCtrManager()
    {
        return ctrManager_;
    }

    boost::shared_ptr<DupDType>& GetDupManager()
    {
        return dupManager_;
    }

    void onIndexUpdated(size_t docNum);

    const faceted::PropValueTable* GetPropValueTable(const std::string& propName) const;

    GroupLabelLogger* GetGroupLabelLogger(const std::string& propName)
    {
        return groupLabelLoggerMap_[propName];
    }

    const MerchantScoreManager* GetMerchantScoreManager() const
    {
        return merchantScoreManager_;
    }

    CustomRankManager* GetCustomRankManager()
    {
        return customRankManager_;
    }

    ProductScorerFactory* GetProductScorerFactory()
    {
        return productScorerFactory_;
    }

    boost::shared_ptr<SearchManager>& GetSearchManager()
    {
        return searchManager_;
    }

    ProductScoreManager* GetProductScoreManager()
    {
        return productScoreManager_;
    }

    const GroupLabelKnowledge* GetGroupLabelKnowledge() const
    {
        return groupLabelKnowledge_;
    }

    const faceted::GroupFilterBuilder* GetGroupFilterBuilder() const
    {
        return groupFilterBuilder_;
    }

    ProductRankerFactory* GetProductRankerFactory()
    {
        return productRankerFactory_;
    }

    NumericPropertyTableBuilder* GetNumericTableBuilder()
    {
        return numericTableBuilder_;
    }

private:
    class WordPriorityQueue_ : public izenelib::util::PriorityQueue<ResultT>
    {
    public:
        WordPriorityQueue_(size_t s)
        {
            initialize(s);
        }
    protected:
        bool lessThan(const ResultT& o1, const ResultT& o2) const
        {
            return (o1.first < o2.first);
        }
    };
    DISALLOW_COPY_AND_ASSIGN(MiningManager);

    void printSimilarLabelResult_(uint32_t label_id);

    /**
     * @brief The offline computation interface used for similarity clustering.
     */
    bool computeSimilarity_(izenelib::ir::indexmanager::IndexReader* pIndexReader, const std::vector<std::string>& property_names);

    /**
     * @brief use explicit semantic analysis for similarity
     */
    bool computeSimilarityESA_(const std::vector<std::string>& property_names);

//     void classifyCollection_(izenelib::ir::indexmanager::IndexReader* pIndexReader);

    /// Add Tg result.
    bool addTgResult_(KeywordSearchResult& miaInput);

    bool addQrResult_(KeywordSearchResult& miaInput);

    bool addDupResult_(KeywordSearchResult& miaInput);

    bool addSimilarityResult_(KeywordSearchResult& miaInput);

    bool addFacetedResult_(KeywordSearchResult& miaInput);

    /**
     *@brief The online querying interface for query recommendation.
     *@param primaryTermIdList The primary terms in the query.
     *@param secondaryTermIdList The secondary terms in the query.
     *@param topDocIdList The top docs from the search result.
     *@param queryList The recommended queries as the result.
     */
    bool getRecommendQuery_(const izenelib::util::UString& queryStr,
                            const std::vector<sf1r::docid_t>& topDocIdList, QueryRecommendRep & recommendRep);

    bool isMiningProperty_(const std::string& name);

    bool isRecommendProperty_(const std::string& name);

    void getAllMiningProperties_(
            const Document& document,
            std::vector<izenelib::util::UString>& values,
            std::vector<izenelib::util::UString>& recommendValues,
            std::string& imgUrl
    );

    void normalTermList_(const izenelib::util::UString& text,
                         std::vector<termid_t>& termList);

    void doIDManagerRelease_();

    void doIDManagerInit_();

    bool doTgInfoInit_();

    void doTgInfoRelease_();

    faceted::PropValueTable::pvid_t propValueId_(
            const std::string& propName,
            const std::vector<std::string>& groupPath
    ) const;

    bool getDocList_(
        const std::vector<docid_t>& docIdList,
        std::vector<Document>& docList
    );

    bool initMerchantScoreManager_(const ProductRankingConfig& rankConfig);
    bool initGroupLabelKnowledge_(const ProductRankingConfig& rankConfig);
    bool initProductScorerFactory_(const ProductRankingConfig& rankConfig);
    bool initProductRankerFactory_(const ProductRankingConfig& rankConfig);

public:
    /// Should be initialized after construction
    static std::string system_resource_path_;
    static std::string system_working_path_;

private:

    /** Global variables */
    std::string collectionDataPath_;
    std::string queryDataPath_;
    CollectionPath collectionPath_;

    std::string collectionName_;
    const DocumentSchema& documentSchema_;

    MiningConfig miningConfig_;
    IndexBundleSchema indexSchema_;
    MiningSchema mining_schema_;
    std::string basicPath_;
    std::string mainPath_;
    std::string backupPath_;
    std::string kpe_res_path_;
    std::string rig_path_;

    /**all analyzer used in mining manager */
    idmlib::util::IDMAnalyzer* analyzer_;
    idmlib::util::IDMAnalyzer* c_analyzer_;
    idmlib::util::IDMAnalyzer* kpe_analyzer_;

    std::string id_path_;

    boost::shared_ptr<DocumentManager> document_manager_;
    boost::shared_ptr<LAManager> laManager_;
    boost::shared_ptr<IndexManager> index_manager_;
    boost::shared_ptr<SearchManager> searchManager_;
    boost::shared_ptr<SearchCache> searchCache_;

    /** TG */
    TaxonomyInfo* tgInfo_;
    boost::shared_ptr<LabelManager> labelManager_;
    boost::shared_ptr<TaxonomyGenerationSubManager> tgManager_;
    boost::shared_ptr<SimCollectorType> label_sim_collector_;
    std::string tg_path_;
    std::string tg_label_path_;
    std::string tg_label_sim_path_;
    std::string tg_label_sim_table_path_;

    /** QR */
    boost::shared_ptr<RecommendManager> rmDb_;
    boost::shared_ptr<QueryRecommendSubmanager> qrManager_;
    boost::shared_ptr<QueryCorrectionSubmanager> qcManager_;

    /** DUPD */
    boost::shared_ptr<DupDType> dupManager_;
    std::string dupd_path_;

    /** SIM */
//     TIRDatabase* termIndex_;
    boost::shared_ptr<sim::SimilarityIndex> similarityIndex_;
    std::string sim_path_;
    boost::shared_ptr<idmlib::sim::DocSimOutput> similarityIndexEsa_;

    /** FACETED */
    boost::shared_ptr<faceted::OntologyManager> faceted_;
    std::string faceted_path_;
    boost::shared_ptr<izenelib::ir::idmanager::IDManager> idManager_;

    /** CTR */
    boost::shared_ptr<faceted::CTRManager> ctrManager_;

    NumericPropertyTableBuilder* numericTableBuilder_;

    /** GROUP BY */
    faceted::GroupManager* groupManager_;

    /** ATTR BY */
    faceted::AttrManager* attrManager_;

    faceted::GroupFilterBuilder* groupFilterBuilder_;

    /** property name => group label click logger */
    typedef std::map<std::string, GroupLabelLogger*> GroupLabelLoggerMap;
    GroupLabelLoggerMap groupLabelLoggerMap_;

    /** Merchant Score */
    MerchantScoreManager* merchantScoreManager_;

    /** Doc Id Converter for Custom Docs */
    CustomDocIdConverter* customDocIdConverter_;

    /** Custom Rank Manager */
    CustomRankManager* customRankManager_;

    OfflineProductScorerFactory* offlineScorerFactory_;

    /** Product Score Table Manager */
    ProductScoreManager* productScoreManager_;

    /** the knowledge of top labels for category boosting */
    GroupLabelKnowledge* groupLabelKnowledge_;

    /** Product Score Factory */
    ProductScorerFactory* productScorerFactory_;

    /** For Merchant Diversity */
    ProductRankerFactory* productRankerFactory_;

    /** TDT */
    std::string tdt_path_;
    TdtStorageType* tdt_storage_;
    NaiveTopicDetector* topicDetector_;

    /** SUMMARIZATION */
    std::string summarization_path_;
    MultiDocSummarizationSubManager* summarizationManagerTask_;

    /** Suffix Match */
    std::string suffix_match_path_;
    SuffixMatchManager* suffixMatchManager_;
    IncrementalManager* incrementalManager_;

    /** Product Matcher */
    std::vector<boost::regex> match_category_restrict_;

    /** Product Query Categorizer */
    QueryCategorizer* product_categorizer_;
    /** KV */
    std::string kv_path_;
    KVSubManager* kvManager_;

    /** MiningTaskBuilder */
    MiningTaskBuilder* miningTaskBuilder_;

    boost::atomic_bool hasDeletedDocDuringMining_;
};

}
#endif
