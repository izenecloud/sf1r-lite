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
#include "custom-rank-manager/CustomRankValue.h"
#include "merchant-score-manager/MerchantScore.h"
#include "ad-index-manager/AdIndexManager.h"

#include <common/ResultType.h>
#include <configuration-manager/PropertyConfig.h>
#include <query-manager/SearchKeywordOperation.h>
#include <configuration-manager/MiningConfig.h>
#include <configuration-manager/MiningSchema.h>
#include <configuration-manager/CollectionPath.h>
#include <ir/id_manager/IDManager.h>
#include <util/cronexpression.h>
#include <util/scheduler.h>
#include <util/ThreadModel.h>
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

namespace util
{
template <class T> class ContainerSwitch;
class IDMAnalyzer;
}

}


namespace sf1r
{

class GroupLabelLogger;
class Document;
class DocumentManager;
class LAManager;
class InvertedIndexManager;
class SearchManager;
class SearchCache;
class MerchantScoreManager;
class CustomRankManager;
class CustomDocIdConverter;
class ProductScorerFactory;
class ProductScoreManager;
class OfflineProductScorerFactory;
class CategoryClassifyTable;
class TitleScoreList;
class ProductForwardManager;
class ProductRankerFactory;
class SuffixMatchManager;
class MiningTaskBuilder;
class MultiThreadMiningTaskBuilder;
class GroupLabelKnowledge;
class NumericPropertyTableBuilder;
class RTypeStringPropTableBuilder;
class ZambeziManager;
class AdIndexManager;
class ProductTokenizer;

namespace faceted
{
class GroupManager;
class AttrManager;
class OntologyManager;
class CTRManager;
class GroupFilterBuilder;
class AttrTable;
}


/**
 * @brief MiningManager manages all the data mining tasks.
 * Those tasks needs heavy offline mining computation. Also, it manages the online query to
 * to the mining results.  Currently, it includes  taxonomy generation, duplication detection,
 * query recommendation and similarity clustering.
 */
class MiningManager
{
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
                  const boost::shared_ptr<InvertedIndexManager>& index_manager,
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

    bool DoMiningCollection(int64_t timestamp);
    bool DoMiningCollectionFromAPI();

    bool DOMiningTask(int64_t timestamp);

    void DoContinue();

    const MiningSchema& getMiningSchema() const { return mining_schema_; }


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
            bool isAnalyzeQuery,
            UString& analyzedQuery,
            std::string& pruneQuery,
            DistKeywordSearchInfo& distSearchInfo,
            faceted::GroupParam::GroupLabelScoreMap& topLabelMap);

    void close();

    boost::shared_ptr<DocumentManager>& GetDocumentManager()
    {
        return  document_manager_;
    }

    void onIndexUpdated(size_t docNum);

    const faceted::PropValueTable* GetPropValueTable(const std::string& propName) const;

    const faceted::AttrTable* GetAttrTable() const;

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

    CategoryClassifyTable* GetCategoryClassifyTable()
    {
        return categoryClassifyTable_;
    }

    TitleScoreList* GetTitleScoreList()
    {
        return titleScoreList_;
    }

    ProductForwardManager* GetProductForwardManger()
    {
        return productForwardManager_;
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

    faceted::AttrManager* GetAttributeManager()
    {
        return attrManager_;
    }

    void updateMergeFuzzyIndex(int calltype);

    RTypeStringPropTableBuilder* GetRTypeStringPropTableBuilder()
    {
        return rtypeStringPropTableBuilder_;
    }

    SuffixMatchManager* getSuffixManager()
    {
        return suffixMatchManager_;
    }

    AdIndexManager* getAdIndexManager()
    {
        return adIndexManager_;
    }

    ProductTokenizer* getProductTokenizer()
    {
        return productTokenizer_;
    }

private:
    DISALLOW_COPY_AND_ASSIGN(MiningManager);

    bool isMiningProperty_(const std::string& name);

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

    bool propValuePaths_(
        const std::string& propName,
        const std::vector<faceted::PropValueTable::pvid_t>& pvIds,
        std::vector<std::vector<std::string> >& paths,
        bool isLock);

    faceted::PropValueTable::pvid_t propValueId_(
            const std::string& propName,
            const std::vector<std::string>& groupPath
    ) const;

    bool getDocList_(
        const std::vector<docid_t>& docIdList,
        std::vector<Document>& docList
    );

    void getGroupAttrRep_(
        const std::vector<std::pair<double, uint32_t> >& res_list,
        faceted::GroupParam& groupParam,
        faceted::GroupRep& groupRep,
        sf1r::faceted::OntologyRep& attrRep,
        const std::string& topPropName,
        faceted::GroupParam::GroupLabelScoreMap& topLabelMap);

    bool initMerchantScoreManager_(const ProductRankingConfig& rankConfig);
    bool initGroupLabelKnowledge_(const ProductRankingConfig& rankConfig);
    bool initCategoryClassifyTable_(const ProductRankingConfig& rankConfig);
    bool initProductScorerFactory_(const ProductRankingConfig& rankConfig);
    bool initProductRankerFactory_(const ProductRankingConfig& rankConfig);
    bool initTitleRelevanceScore_(const ProductRankingConfig& rankConfig);
    bool initProductForwardManager_();

    bool initAdIndexManager_(AdIndexConfig& adIndexConfig);

    const std::string& getOfferItemCountPropName_() const;

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

    std::string id_path_;

    boost::shared_ptr<DocumentManager> document_manager_;
    boost::shared_ptr<LAManager> laManager_;
    boost::shared_ptr<InvertedIndexManager> index_manager_;
    boost::shared_ptr<SearchManager> searchManager_;
    boost::shared_ptr<SearchCache> searchCache_;

    boost::shared_ptr<izenelib::ir::idmanager::IDManager> idManager_;

    NumericPropertyTableBuilder* numericTableBuilder_;

    RTypeStringPropTableBuilder* rtypeStringPropTableBuilder_;

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

    /** Table stores one classified category for each doc */
    CategoryClassifyTable* categoryClassifyTable_;

    /** list stores all the documents' productTokenizer score*/
    TitleScoreList* titleScoreList_;

    /** The forward index, for B5MA only*/
    ProductForwardManager* productForwardManager_;

    /** the knowledge of top labels for category boosting */
    GroupLabelKnowledge* groupLabelKnowledge_;

    /** Product Score Factory */
    ProductScorerFactory* productScorerFactory_;

    /** For Merchant Diversity */
    ProductRankerFactory* productRankerFactory_;


    /** Suffix Match */
    std::string suffix_match_path_;
    SuffixMatchManager* suffixMatchManager_;
    ProductTokenizer* productTokenizer_;

    /** AdIndexManager */
    AdIndexManager* adIndexManager_;

    /** MiningTaskBuilder */
    MiningTaskBuilder* miningTaskBuilder_;

    /** MultiThreadMiningTaskBuilder */
    MultiThreadMiningTaskBuilder* multiThreadMiningTaskBuilder_;

    izenelib::util::CronExpression cronExpression_;

    static bool startSynonym_;
    long lastModifiedTime_;

    boost::atomic_bool hasDeletedDocDuringMining_;
};

}
#endif
