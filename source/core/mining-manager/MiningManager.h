
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

#include "faceted-submanager/prop_value_table.h" // pvid_t

#include <common/ResultType.h>
#include <configuration-manager/PropertyConfig.h>
#include <query-manager/ActionItem.h>
#include <configuration-manager/MiningConfig.h>
#include <configuration-manager/MiningSchema.h>
#include <ir/id_manager/IDManager.h>
#include <boost/shared_ptr.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/thread/mutex.hpp>
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

class DupDetector2;
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
class IndexManager;
class SearchManager;
class MultiDocSummarizationSubManager;

namespace sim
{
class SimilarityIndex;
}

namespace faceted
{
class GroupManager;
class AttrManager;
class PropertyDiversityReranker;
class OntologyManager;
class CTRManager;
}

/**
 * @brief MiningManager manages all the data mining tasks.
 * Those tasks needs heavy offline mining computation. Also, it manages the online query to
 * to the mining results.  Currently, it includes  taxonomy generation, duplication detection,
 * query recommendation and similarity clustering.
 */
class MiningManager : public boost::noncopyable
{
typedef DupDetector2 DupDType;
typedef idmlib::util::ContainerSwitch<idmlib::tdt::Storage> TdtStorageType;
typedef idmlib::sim::TermSimilarityTable<uint32_t> SimTableType;
typedef idmlib::sim::SimOutputCollector<SimTableType> SimCollectorType;
typedef idmlib::sim::TermSimilarity<SimCollectorType, uint32_t, uint32_t, uint32_t> TermSimilarityType;
public:
    /**
     * @brief The constructor of MiningManager.
     * @param miningManagerConfig specifies the user configured parameters from configuration file.
     * @param idManager used for generating term Ids.
     * @param docIdManager used for generating docIds in parsing the SCD file.
     */
    MiningManager(const std::string& collectionDataPath, const std::string& queryDataPath,
                  const boost::shared_ptr<DocumentManager>& documentManager,
                  const boost::shared_ptr<IndexManager>& index_manager,
                  const boost::shared_ptr<SearchManager>& searchManager,
                  const boost::shared_ptr<izenelib::ir::idmanager::IDManager>& idManager,
                  const std::string& collectionName,
                  const schema_type& schema,
                  const MiningConfig& miningConfig,
                  const MiningSchema& miningSchema);

    ~MiningManager();

    bool open();

    bool DoMiningCollection();


    /**
     * @brief The online querying interface.
     */
    bool getMiningResult(KeywordSearchResult& miaInput);

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
     * @param docIdList the docs inputed.
     * @param removedDocs the docs removed.
     */
    bool getUniqueDocIdList(const std::vector<uint32_t>& docIdList,
                            std::vector<uint32_t>& cleanDocs);

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
     * @param propName the property name for the group labels to get
     * @param groupPath the path of the group label
     * @return true for success, false for failure
     */
    bool setTopGroupLabel(
        const std::string& query,
        const std::string& propName,
        const std::vector<std::string>& groupPath
    );

    bool GetTdtInTimeRange(const izenelib::util::UString& start, const izenelib::util::UString& end, std::vector<izenelib::util::UString>& topic_list);

    bool GetTdtInTimeRange(const boost::gregorian::date& start, const boost::gregorian::date& end, std::vector<izenelib::util::UString>& topic_list);

    bool GetTdtTopicInfo(const izenelib::util::UString& text, std::pair<idmlib::tdt::TrackResult, std::vector<izenelib::util::UString> >& info);

    void GetRefinedQuery(const izenelib::util::UString& query, izenelib::util::UString& result);

    void InjectQueryCorrection(const izenelib::util::UString& query, const izenelib::util::UString& result);

    void FinishQueryCorrectionInject();

    void InjectQueryRecommend(const izenelib::util::UString& query, const izenelib::util::UString& result);

    void FinishQueryRecommendInject();

    void close();

    boost::shared_ptr<faceted::OntologyManager> GetFaceted()
    {
        return faceted_;
    }

    boost::shared_ptr<TaxonomyGenerationSubManager> GetTgManager()
    {
        return tgManager_;
    }

    boost::shared_ptr<DocumentManager> GetDocumentManager()
    {
        return  document_manager_;
    }

    boost::shared_ptr<RecommendManager> GetRecommendManager()
    {
        return rmDb_;
    }


    void onIndexUpdated(size_t docNum);
private:


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
     * @brief Building multi document summarization offline
     */
    bool computeSummarization_();

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

    void getAllMiningProperties_(const Document& document, std::vector<izenelib::util::UString>& values, std::vector<izenelib::util::UString>& recommendValues, std::string& imgUrl);



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

public:
    /// Should be initialized after construction
    static std::string system_resource_path_;
    static std::string system_working_path_;

private:

    /** Global variables */
    std::string collectionDataPath_;
    std::string queryDataPath_;
    std::string collectionName_;
    const schema_type& schema_;

    MiningConfig miningConfig_;
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
    boost::shared_ptr<IndexManager> index_manager_;
    boost::shared_ptr<SearchManager> searchManager_;

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

    /** GROUP BY */
    faceted::GroupManager* groupManager_;

    /** ATTR BY */
    faceted::AttrManager* attrManager_;

    /** GROUP RERANKER */
    faceted::PropertyDiversityReranker* groupReranker_;

    /** property name => group label click logger */
    typedef std::map<std::string, GroupLabelLogger*> GroupLabelLoggerMap;
    GroupLabelLoggerMap groupLabelLoggerMap_;

    /** TDT */
    std::string tdt_path_;
    TdtStorageType* tdt_storage_;

    /** SUMMARIZATION */
    std::string summarization_path_;
    MultiDocSummarizationSubManager* summarizationManager_;
};

}
#endif
