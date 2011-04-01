
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


#ifndef _MINING_MANAGER_H_
#define _MINING_MANAGER_H_


#include "MiningManagerDef.h"
#include "status.h"
#include "MiningQueryLogHandler.h"
#include "duplicate-detection-submanager/DupDetector2.h"
#include "query-recommend-submanager/QueryRecommendSubmanager.h"
#include "query-recommend-submanager/RecommendManager.h"
#include "taxonomy-generation-submanager/TaxonomyGenerationSubManager.h"
#include "taxonomy-generation-submanager/TaxonomyInfo.h"
#include "taxonomy-generation-submanager/label_similarity.h"
#include "similarity-detection-submanager/SimilarityIndex.h"
#include "query-correction-submanager/QueryCorrectionSubmanager.h"
#include "faceted-submanager/ontology_manager.h"
#include "MiningManagerDef.h"
#include <common/ResultType.h>
#include <common/Status.h>
#include <query-manager/ActionItem.h>
#include <configuration-manager/PropertyConfig.h>
#include <configuration-manager/MiningConfig.h>
#include <document-manager/Document.h>
#include <index-manager/IndexManager.h>
#include <am/3rdparty/rde_hash.h>
#include <am/sdb_hash/sdb_hash.h>
#include <util/izene_log.h>
#include <idmlib/util/idm_analyzer.h>
#include <idmlib/similarity/term_similarity.h>
#include <ir/id_manager/IDManager.h>
#include <boost/shared_ptr.hpp>
#include <boost/optional.hpp>

#include <string>
#include <map>
#include <boost/thread/mutex.hpp>

using namespace izenelib::ir::idmanager;
namespace sf1r
{

namespace faceted
{
class GroupManager;
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

public:
    /**
     * @brief The constructor of MiningManager.
     * @param miningManagerConfig specifies the user configured parameters from configuration file.
     * @param idManager used for generating term Ids.
     * @param docIdManager used for generating docIds in parsing the SCD file.
     */
    MiningManager(const std::string& collectionDataPath, const std::string& queryDataPath,
                  const boost::shared_ptr<LAManager>& laManager,
                  const boost::shared_ptr<DocumentManager>& documentManager,
                  const boost::shared_ptr<IndexManager>& index_manager,
                  const std::string& collectionName,
                  const schema_type& schema,
                  const MiningConfig& miningConfig,
                  const MiningSchema& miningSchema);

    MiningManager(const std::string& collectionDataPath, const std::string& queryDataPath,
                  const boost::shared_ptr<LAManager>& laManager,
                  const boost::shared_ptr<DocumentManager>& documentManager,
                  const boost::shared_ptr<IndexManager>& index_manager,
                  const std::string& collectionName,
                  const schema_type& schema,
                  const MiningConfig& miningConfig,
                  const MiningSchema& miningSchema,
                  const boost::shared_ptr<IDManager>idManager);

    ~MiningManager();

    bool open();

//     void setConfigClient(const boost::shared_ptr<ConfigurationManagerClient>& configClient);

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
     * @brief Get group representation for a property list.
     * @param docIdList a list of doc id, in which doc count is calculated for each property value
     * @param groupPropertyList a list of property name.
     * @param groupRep a list, each element is a label tree for a property,
     *				   each label contains doc count for a property value.
     * @return true for success, false for failure.
     */
    bool getGroupRep(
        const std::vector<unsigned int>& docIdList,
        const std::vector<std::string>& groupPropertyList,
        faceted::OntologyRep& groupRep
    );

//     void getMiningStatus(Status& status);

    void close();

    boost::shared_ptr<faceted::OntologyManager> GetFaceted()
    {
        return faceted_;
    }
private:


    void printSimilarLabelResult_(uint32_t label_id);

    /**
     * @brief The offline computation interface used for similarity clustering.
     */
    bool computeSimilarity_(izenelib::ir::indexmanager::IndexReader* pIndexReader, const std::vector<std::string>& property_names);

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

    void getAllMiningProperties_(const Document& document, std::vector<izenelib::util::UString>& values, std::vector<izenelib::util::UString>& recommendValues, std::string& imgUrl);


//     void analyzeLabelRelation();

    void replicatingLabel_();



    void normalTermList_(const izenelib::util::UString& text,
                         std::vector<termid_t>& termList);

    void doIDManagerRelease_();

    void doIDManagerInit_();



    bool doTgInfoInit_();

    void doTgInfoRelease_();

public:
    /// Should be initialized after construction
    std::string system_resource_path_;

private:

    /** Global variables */
    std::string collectionDataPath_;
    std::string queryDataPath_;
    std::string collectionName_;
    const schema_type& schema_;

    MiningConfig miningConfig_;
    MiningSchema mining_schema_;
//     boost::shared_ptr<ConfigurationManagerClient> configClient_;
    std::string basicPath_;
    std::string mainPath_;
    std::string backupPath_;

    /** Global status */
//     boost::shared_ptr<MiningStatus> status_;
//     bool bProcessBroken_;

    /**id manager */
    boost::shared_ptr<LAManager> laManager_;
    idmlib::util::IDMAnalyzer* analyzer_;
    idmlib::util::IDMAnalyzer* kpe_analyzer_;
//     MiningIDManager* idManager_;

    std::string id_path_;

    boost::shared_ptr<DocumentManager> document_manager_;
    boost::shared_ptr<IndexManager> index_manager_;
    /** TG */
    TaxonomyInfo* tgInfo_;
    boost::shared_ptr<LabelManager> labelManager_;
    boost::shared_ptr<TaxonomyGenerationSubManager> tgManager_;
    boost::shared_ptr<LabelSimilarity::SimTableType> label_sim_table_;
    std::string tg_path_;
    std::string tg_label_path_;
    std::string tg_label_sim_path_;
    std::string tg_label_sim_table_path_;

    /** QR */
    boost::shared_ptr<RecommendManager> rmDb_;
    boost::shared_ptr<QueryRecommendSubmanager> qrManager_;
    std::string qr_path_;

    /** DUPD */
    boost::shared_ptr<DupDType> dupManager_;
    std::string dupd_path_;

    /** SIM */
//     TIRDatabase* termIndex_;
    boost::shared_ptr<SimilarityIndex> similarityIndex_;
    std::string sim_path_;
    /** FACETED */
    boost::shared_ptr<faceted::OntologyManager> faceted_;
    std::string faceted_path_;
    boost::shared_ptr<IDManager>idManager_;

    /** GROUP BY */
    faceted::GroupManager* groupManager_;
};

}
#endif
