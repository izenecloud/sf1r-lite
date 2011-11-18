/**
 * @file WorkerService.h
 * @author Zhongxia Li
 * @date Jul 5, 2011
 * @brief Worker Service encapsulated service interfaces (such as Search, Mining..) of SF1.
 */
#ifndef WORKER_SERVICE_H_
#define WORKER_SERVICE_H_

#include <query-manager/ActionItem.h>
#include <query-manager/SearchKeywordOperation.h>
#include <la-manager/AnalysisInformation.h>
#include <common/ResultType.h>

// index task
#include <directory-manager/DirectoryRotator.h>
#include <configuration-manager/PropertyConfig.h>
#include <configuration-manager/ConfigurationTool.h>
#include <document-manager/Document.h>
#include <document-manager/text-summarization-submanager/TextSummarizationSubManager.h>
#include <common/Status.h>
#include <common/IndexingProgress.h>
#include <common/ScdParser.h>
#include <common/ScdWriterController.h>
#include <ir/id_manager/IDManager.h>
#include <ir/index_manager/index/IndexerDocument.h>

#include <util/get.h>
#include <net/aggregator/JobInfo.h>
#include <ir/id_manager/IDManager.h>
#include <question-answering/QuestionAnalysis.h>

#include <boost/shared_ptr.hpp>

namespace sf1r
{
using izenelib::ir::idmanager::IDManager;

typedef std::vector<std::pair<uint64_t, float> > SimilarDocIdListType;

class IndexBundleConfiguration;
class IndexSearchService;
class MiningSearchService;
class RecommendSearchService;
class User;
class IndexManager;
class DocumentManager;
class LAManager;
class SearchManager;
class MiningManager;
// index task
class ScdWriterController;
class IndexHooker;
class WorkerService
{
    typedef uint32_t CharacterOffset;

public:
    WorkerService(
            IndexBundleConfiguration* bundleConfig,
            DirectoryRotator& directoryRotator);

public:
    /**
     * Worker services (interfaces)
     * 1) add to WorkerServer to provide remote procedure call
     * 2) add to AggregatorManager::initLocalWorkerCaller to provide in-procedure call
     * @{
     */

    /// search
    bool getDistSearchInfo(const KeywordSearchActionItem& actionItem, DistKeywordSearchInfo& resultItem);

    bool getDistSearchResult(const KeywordSearchActionItem& actionItem, DistKeywordSearchResult& resultItem);

    bool getSummaryMiningResult(const KeywordSearchActionItem& actionItem, KeywordSearchResult& resultItem);

    /// documents
    bool getDocumentsByIds(const GetDocumentsByIdsActionItem& actionItem, RawTextResultFromSIA& resultItem);

    bool getInternalDocumentId(const izenelib::util::UString& actionItem, uint64_t& resultItem);

    /// mining
    bool getSimilarDocIdList(uint64_t& documentId, uint32_t& maxNum, SimilarDocIdListType& result);

    bool clickGroupLabel(const ClickGroupLabelActionItem& actionItem, bool& ret);

    bool visitDoc(const uint32_t& docId, bool& ret);

    /// index
    bool index(const unsigned int& numdoc, bool& ret);

    /** @} */

    bool doLocalSearch(const KeywordSearchActionItem& actionItem, KeywordSearchResult& resultItem);

private:
    template <typename ResultItemType>
    bool getSearchResult_(
            const KeywordSearchActionItem& actionItem,
            ResultItemType& resultItem,
            bool isDistributedSearch = true);

    bool getSummaryMiningResult_(
            const KeywordSearchActionItem& actionItem,
            KeywordSearchResult& resultItem,
            bool isDistributedSearch = true);

    void analyze_(const std::string& qstr, std::vector<izenelib::util::UString>& results);

    template <typename ResultItemT>
    bool buildQuery(
            SearchKeywordOperation& actionOperation,
            std::vector<std::vector<izenelib::util::UString> >& propertyQueryTermList,
            ResultItemT& resultItem,
            PersonalSearchInfo& personalSearchInfo);

    template<typename ActionItemT, typename ResultItemT>
    bool  getResultItem(ActionItemT& actionItem, const std::vector<sf1r::docid_t>& docsInPage,
        const vector<vector<izenelib::util::UString> >& propertyQueryTermList, ResultItemT& resultItem);

    template <typename ResultItemType>
    bool removeDuplicateDocs(
            const KeywordSearchActionItem& actionItem,
            ResultItemType& resultItem);

    /// @{
    /// index task

    bool buildCollection(unsigned int numdoc);

    bool getPropertyValue_( const PropertyValue& value, std::string& valueStr );

    bool backup_();

    bool doBuildCollection_(
            const std::string& scdFile,
            int op,
            uint32_t numdoc
        );

    bool insertOrUpdateSCD_(
            ScdParser& parser,
            bool isInsert,
            uint32_t numdoc,
            time_t timestamp
        );

    bool createUpdateDocId_(
        const izenelib::util::UString& scdDocId,
        bool rType,
        docid_t& oldId,
        docid_t& newId
    );

    bool createInsertDocId_(
        const izenelib::util::UString& scdDocId,
        docid_t& newId
    );

    bool deleteSCD_(ScdParser& parser, time_t timestamp);

    bool insertDoc_(Document& document, IndexerDocument& indexDocument, time_t timestamp);

    bool updateDoc_(Document& document, IndexerDocument& indexDocument, time_t timestamp, bool rType);

    bool deleteDoc_(docid_t docid, time_t timestamp);

    void saveSourceCount_(int op);

    bool prepareDocument_(
            SCDDoc& doc,
            Document& document,
            docid_t& oldId,
            bool& rType,
            std::map<std::string, pair<PropertyDataType, izenelib::util::UString> >& rTypeFieldValue,
            std::string& source,
            bool insert = true
    );

    bool prepareIndexDocument_(
            docid_t oldId,
            const Document& document,
            IndexerDocument& indexDocument
    );

    bool preparePartialDocument_(
            Document& document,
            IndexerDocument& oldIndexDocument
    );

    bool completePartialDocument_(docid_t oldId, Document& doc);

    bool checkSeparatorType_(
        const izenelib::util::UString& propertyValueStr,
        izenelib::util::UString::EncodingType encoding,
        char separator
    );

    bool checkRtype_(
            SCDDoc& doc,
            std::map<std::string, pair<PropertyDataType, izenelib::util::UString> >& rTypeFieldValue
    );

    bool makeForwardIndex_(
        const izenelib::util::UString& text,
        const std::string& propertyName,
        unsigned int propertyId,
        const AnalysisInfo& analysisInfo
    );

    bool makeSentenceBlocks_(
        const izenelib::util::UString& text,
        const unsigned int numOfSummary,
        const unsigned int maxDisplayLength,
        std::vector<CharacterOffset>& sentenceOffsetList
    );

    /// @}

private:
    IndexBundleConfiguration* bundleConfig_;
    RecommendSearchService* recommendSearchService_;

    boost::shared_ptr<LAManager> laManager_;
    boost::shared_ptr<IDManager> idManager_;
    boost::shared_ptr<DocumentManager> documentManager_;
    boost::shared_ptr<IndexManager> indexManager_;
    boost::shared_ptr<SearchManager> searchManager_;
    boost::shared_ptr<MiningManager> miningManager_;
    ilplib::qa::QuestionAnalysis* pQA_;

    AnalysisInfo analysisInfo_;

    // index task
    DirectoryRotator& directoryRotator_;
    PropertyConfig dateProperty_;
    config_tool::PROPERTY_ALIAS_MAP_T propertyAliasMap_;

    ScdWriterController* scd_writer_;
    TextSummarizationSubManager summarizer_;

    unsigned int collectionId_;
    IndexingProgress indexProgress_;
    bool checkInsert_;
    unsigned int numDeletedDocs_;
    unsigned int numUpdatedDocs_;

    Status indexStatus_;

    std::vector<boost::shared_ptr<LAInput> > laInputs_;
    std::vector<string> propertyList_;
    std::map<std::string, uint32_t> productSourceCount_;
    boost::shared_ptr<IndexHooker> hooker_;

    friend class WorkerServer;
    friend class IndexSearchService;
    friend class IndexBundleActivator;
    friend class MiningBundleActivator;
    friend class ProductBundleActivator;
    friend class RecommendBundleActivator;
};

}

#endif /* WORKER_SERVICE_H_ */
